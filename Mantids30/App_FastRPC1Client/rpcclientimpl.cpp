#include "rpcclientimpl.h"
#include "globals.h"
#include <cstdint>
#include <inttypes.h>
#include <memory>
#include <string>
#include <thread>

#include <Mantids30/Helpers/crypto.h>
#include <Mantids30/Helpers/file.h>
#include <Mantids30/Net_Sockets/streams_cryptochallenge.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <utility>

using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Applications::FastRPC1;
using namespace Mantids30::Program;

RPCClientImpl::RPCClientImpl()
{
    getClientConfigCmd="getClientConfig";
    updateClientConfigLoadTimeCmd="updateClientConfigLoadTime";
    failedToRetrieveC2Config = false;
}

RPCClientImpl::~RPCClientImpl()
{

}

void RPCClientImpl::runRPClient0(RPCClientImpl *rpcImpl)
{
    rpcImpl->runRPClient();
}

void RPCClientImpl::runRPClient()
{
    uint16_t remotePort = Globals::getLC_C2RemotePort();
    std::string remoteAddr = Globals::getLC_C2RemoteAddress();
    int secsBetweenConnections = Globals::getLC_C2TimeBetweenConnections();

    addMethods();
    std::string caCertPath = Globals::getLC_TLSCAFilePath();
    std::string privKeyPath = Globals::getLC_TLSKeyFilePath();
    std::string pubCertPath = Globals::getLC_TLSCertFilePath();

    for (;;)
    {
        std::shared_ptr<Mantids30::Network::Sockets::Socket_TLS> sockRPCClient = std::make_shared<Mantids30::Network::Sockets::Socket_TLS>();

        if (!Globals::getLC_C2UsePSK())
        {
            // Set the SO default security level:
            sockRPCClient->m_keys.setSecurityLevel(-1);
            
            if (!sockRPCClient->m_keys.loadCAFromPEMFile(  caCertPath.c_str() ))
            {
                LOG_APP->log0(__func__,Logs::LEVEL_ERR, "Error starting RPC Connector to %s:%" PRIu16 ": Bad/Unaccesible TLS Certificate Authority (%s)", remoteAddr.c_str(), remotePort, caCertPath.c_str());
                _exit(-3);
            }

            auto masterKey = Globals::getMasterKey();

            // Check if using passphrase
            if (  !Globals::getLC_TLSPhraseFileForPrivateKey().empty() )
            {
                bool ok = false;
                // Load Key
                std::string keyPassPhrase = Mantids30::Helpers::Crypto::AES256DecryptB64( Mantids30::Helpers::File::loadFileIntoString( Globals::getLC_TLSPhraseFileForPrivateKey() )
                                                                            ,(char *)masterKey->data,masterKey->length,&ok
                                                                            );
                
                if (!sockRPCClient->m_keys.loadPrivateKeyFromPEMFileEP(  privKeyPath.c_str(), keyPassPhrase.c_str() ))
                {
                    LOG_APP->log0(__func__,Logs::LEVEL_ERR, "Error starting RPC Connector to %s:%" PRIu16 ": Bad/Unaccesible TLS Private Certificate / Passphrase (%s)", remoteAddr.c_str(), remotePort, privKeyPath.c_str());
                    _exit(-35);
                }
            }
            else
            {
                if (!sockRPCClient->m_keys.loadPrivateKeyFromPEMFile(  privKeyPath.c_str() ))
                {
                    LOG_APP->log0(__func__,Logs::LEVEL_ERR, "Error starting RPC Connector to %s:%" PRIu16 ": Bad/Unaccesible TLS Private Certificate (%s)", remoteAddr.c_str(), remotePort, privKeyPath.c_str());
                    _exit(-3);
                }
            }
            if (!sockRPCClient->m_keys.loadPublicKeyFromPEMFile(  pubCertPath.c_str() ))
            {
                LOG_APP->log0(__func__,Logs::LEVEL_ERR, "Error starting RPC Connector to %s:%" PRIu16 ": Bad/Unaccesible TLS Public Certificate (%s)", remoteAddr.c_str(), remotePort, pubCertPath.c_str());
                _exit(-3);
            }

            LOG_APP->log0(__func__,Logs::LEVEL_INFO, "PKI X.509 credentials loaded from the internal storage");
        }
        else
        {
            // Load Key
            bool ok;
            auto idpsk = loadPSK();
            sockRPCClient->m_keys.setUsingPSK();
            sockRPCClient->m_keys.loadPSKAsClient(idpsk.id, idpsk.psk);
        }

        LOG_APP->log0(__func__,Logs::LEVEL_INFO,  "Connecting to RPC Server %s:%" PRIu16 "...", remoteAddr.c_str(), remotePort);

        if ( sockRPCClient->connectTo( remoteAddr.c_str(), remotePort ) )
        {
            LOG_APP->log0(__func__,Logs::LEVEL_INFO,  "RPC Client Connected to server %s:%" PRIu16 " (CN=%s) Using %s", remoteAddr.c_str(), remotePort, sockRPCClient->getTLSPeerCN().c_str(),sockRPCClient->getTLSConnectionCipherName().c_str());

            if (postConnect(sockRPCClient))
            {
                // now is fully connected / authenticated...
                if (failedToRetrieveC2Config)
                    connectedToC2AfterFailingToLoadC2Config();
                fastRPC.processConnection(sockRPCClient,"SERVER");
            }
            LOG_APP->log0(__func__,Logs::LEVEL_WARN,  "RPC Client disconnected from %s:%" PRIu16 " (CN=%s)", remoteAddr.c_str(), remotePort, sockRPCClient->getTLSPeerCN().c_str());
        }
        else
        {
            LOG_APP->log0(__func__,Logs::LEVEL_ERR, "Error connecting to remote RPC Server @%s:%" PRIu16 ": %s",
                          remoteAddr.c_str(), remotePort,
                          sockRPCClient->getLastError().c_str());

            for (const auto & i :sockRPCClient->getTLSErrorsAndClear())
            {
                if (!strstr(i.c_str(),"certificate unknown"))
                    LOG_APP->log1(__func__, remoteAddr.c_str(),Logs::LEVEL_ERR, ">>> TLS Error: %s", i.c_str());
            }
        }

        sleep(secsBetweenConnections);
    }
}

bool RPCClientImpl::retrieveConfigFromLocalFile()
{
    /////////////////////////////////////////////////////////////
    //***********CONFIG RETRIEVE FROM FILE**********
    /////////////////////////////////////////////////////////////
    std::string localConfigPath = Globals::getLC_RemoteConfigFilePath();
    LOG_APP->log0(__func__,Logs::LEVEL_INFO, "Retrieving config from local file: %s",localConfigPath.c_str());

    std::ifstream infile(localConfigPath);

    if (infile.is_open())
    {
        std::string lineInFile;
        if (std::getline(infile, lineInFile))
        {
            std::string sRemoteConfigDecrypted = decryptStr(lineInFile);

            json jRemoteConfig;
            Mantids30::Helpers::JSONReader2 reader;
            bool parsingSuccessful = reader.parse( sRemoteConfigDecrypted, jRemoteConfig );
            if ( parsingSuccessful )
            {
                jRetrievedConfig=jRemoteConfig;
                infile.close();
                return true;
            }
        }
        infile.close();
    }
    return false;
}

bool RPCClientImpl::retrieveConfigFromC2()
{
    /////////////////////////////////////////////////////////////
    //***********INITIALIZE C2 COMM AND CONFIG RETRIEVE**********
    /////////////////////////////////////////////////////////////
    json rpcError;

    failedToRetrieveC2Config = false;

    LOG_APP->log0(__func__,Logs::LEVEL_INFO, "Retrieving config from remote C2.");

    // Try to retrieve the configuration from the C&C. (will try several attempts)
    json jRemoteConfig = fastRPC.runRemoteRPCMethod("SERVER",getClientConfigCmd,{},&rpcError);
    if (rpcError["succeed"].asBool() == true)
    {
        // Translate this config to the configuration file...
        std::string sRemoteConfig = jRemoteConfig.toStyledString();
        std::string sLocalConfig = jRetrievedConfig.toStyledString();

        if (sRemoteConfig == sLocalConfig)
        {
            LOG_APP->log0(__func__,Logs::LEVEL_INFO, "C2 remote/local configuration is the same. Not upgrading.");
            return true;
        }

        if ( sRemoteConfig.size()>7 )
        {
            std::string sRemoteConfigEncrypted = encryptStr(sRemoteConfig);
            std::string localConfigPath = Globals::getLC_RemoteConfigFilePath();

            std::ofstream outfile;
            outfile.open(localConfigPath, std::ios_base::out);
            if (outfile.is_open())
            {
                outfile  << sRemoteConfigEncrypted << "\n";
                outfile.close();

                jRetrievedConfig=jRemoteConfig;

                json ans;
                ans["x"] = fastRPC.runRemoteRPCMethod("SERVER",updateClientConfigLoadTimeCmd,{},&rpcError);

                if ( rpcError["succeed"].asBool() == false )
                {
                    LOG_APP->log0(__func__,Logs::LEVEL_ERR, "Configuration loaded from the remote server, but failed to update the C2 config access time... %s", rpcError["errorMessage"].asCString());
                }

                if ( JSON_ASBOOL(ans,"x",false)==false )
                {
                    LOG_APP->log0(__func__,Logs::LEVEL_ERR, "Configuration loaded from the remote server, but failed to update the C2 config access time.");
                }

                LOG_APP->log0(__func__,Logs::LEVEL_INFO, "C2 configuration written to: %s",localConfigPath.c_str());

                return true;
            }
            else
            {
                LOG_APP->log0(__func__,Logs::LEVEL_ERR, "Failed to write the remote configuration to: %s", localConfigPath.c_str());
            }
        }
        else
        {
            LOG_APP->log0(__func__,Logs::LEVEL_ERR, "Remote configuration from the C2 is not reliable.");
        }
    }
    else
    {
        LOG_APP->log0(__func__,Logs::LEVEL_ERR, "Can't retrieve configuration from the C2: %s", rpcError["errorMessage"].asCString());
    }

    // If the C2 is available in the near future, handle it (recommendation: exit the program).
    failedToRetrieveC2Config = true;


    return false;
}

json RPCClientImpl::getJRetrievedConfig()
{
    return jRetrievedConfig;
}

RPCClientImpl::PSKIdKey RPCClientImpl::loadPSK()
{
    bool ok = false;
    PSKIdKey r;
    std::string encryptedKey = Mantids30::Helpers::File::loadFileIntoString( Globals::getLC_C2PSKSharedKeyFilePath() , &ok);
    auto masterKey = Globals::getMasterKey();
    if (!ok || encryptedKey.empty())
    {
        if (!ok)
        {
            LOG_APP->log0(__func__,Logs::LEVEL_ERR, "Failed to read '%s' RPC-PSK Credentials",Globals::getLC_C2PSKSharedKeyFilePath().c_str() );
        }

        r = defaultPSK();
        if (r.id.empty())
        {
            LOG_APP->log0(__func__,Logs::LEVEL_ERR, "Error in RPC Client: PSK Key content/file not found");
            exit(-330);
        }

        LOG_APP->log0(__func__,Logs::LEVEL_WARN, "Using default RPC-PSK Credentials (id=%s)", r.id.c_str());
    }
    else
    {
        std::string tokenizedKey = Mantids30::Helpers::Crypto::AES256DecryptB64( encryptedKey,(char *)masterKey->data,masterKey->length,&ok );
        std::vector<std::string> keyParts;
        split(keyParts,tokenizedKey,is_any_of(":"),token_compress_on);
        if (!ok || keyParts.size()!=2)
        {
            LOG_APP->log0(__func__,Logs::LEVEL_ERR, "Error in RPC Client: PSK Key not in ID:PSK format");
            _exit(-331);
        }
        r.id = keyParts.at(0);
        r.psk = keyParts.at(1);
        LOG_APP->log0(__func__,Logs::LEVEL_INFO, "RPC-PSK credentials loaded with id='%s' from the internal storage", r.id.c_str());
    }
    return r;
}

RPCClientImpl::PSKIdKey RPCClientImpl::defaultPSK()
{
    return  {"",""};
}
