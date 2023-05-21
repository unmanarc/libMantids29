#pragma once

#include "socket_tcp.h"
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

namespace Mantids29 {
namespace Network {
namespace Sockets {

/**
 * TCP Socket Class
 */
class Socket_TLS : public Socket_TCP
{
public:
    struct sCipherBits
    {
        sCipherBits()
        {
            asymmetricBits = 0;
            symmetricBits = 0;
        }
        int asymmetricBits, symmetricBits;
    };

    class TLSKeyParameters
    {
    public:
        struct PSKClientValue
        {
            PSKClientValue() { m_isUsingPSK = false; }

            ~PSKClientValue()
            {
                std::unique_lock<std::mutex> lock(m_mutex);

                // Erase the PSK data from memory.
                m_psk.resize(m_psk.capacity(), 0);
                memset(&m_psk[0], 0x7F, m_psk.size());
            }

            void setValues(const std::string &identity, const std::string &psk)
            {
                std::unique_lock<std::mutex> lock(m_mutex);

                m_isUsingPSK = true;
                this->m_psk = psk;
                this->m_identity = identity;
            }

            bool m_isUsingPSK;
            std::string m_psk;
            std::string m_identity;
            std::mutex m_mutex;
        };

        struct PSKServerWallet
        {
            typedef bool (*cbPSK)(void *data, const std::string &id, std::string *psk);

            ~PSKServerWallet()
            {
                for (auto &i : m_pskByClientIdMap)
                {
                    // Erase the PSK data from memory.
                    i.second.resize(i.second.capacity(), 0);
                    memset(&i.second[0], 0x7F, i.second.size());
                }
            }

            void operator=(PSKServerWallet &x)
            {
                std::unique_lock<std::mutex> lock1(m_mutex);
                std::unique_lock<std::mutex> lock2(x.m_mutex);

                m_isUsingPSK = x.m_isUsingPSK;
                m_pskByClientIdMap = x.m_pskByClientIdMap;
                m_pskCallback = x.m_pskCallback;
            }

            bool getPSKByClientID(const std::string &id, std::string *psk)
            {
                bool r = false;
                std::unique_lock<std::mutex> lock(m_mutex);
                if (m_pskByClientIdMap.find(id) != m_pskByClientIdMap.end())
                {
                    r = true;
                    *psk = m_pskByClientIdMap[id];
                }
                return r;
            }
            bool setPSKByClientID(const std::string &id, const std::string &psk)
            {
                bool r = false;
                std::unique_lock<std::mutex> lock(m_mutex);
                m_isUsingPSK = true;
                if (m_pskByClientIdMap.find(id) == m_pskByClientIdMap.end())
                {
                    r = true;
                    m_pskByClientIdMap[id] = psk;
                }
                return r;
            }

            /**
             * @brief setPSKCallback Set PSK Callback, warn: you have to manage multithread environment there.
             * @param newCbpsk Callback function
             */
            void setPSKCallback(cbPSK newCbpsk, void *data)
            {
                this->m_data = data;
                m_pskCallback = newCbpsk;
                m_isUsingPSK = true;
            }

            void *m_data = nullptr;
            cbPSK m_pskCallback = nullptr;

            bool m_isUsingPSK = false;
            std::string m_connectedClientID;
            std::map<std::string, std::string> m_pskByClientIdMap;
            std::mutex m_mutex;
        };

        class PSKStaticHdlr
        {
        public:
            PSKStaticHdlr(PSKClientValue *pskClientValues, PSKServerWallet *pskServerValues);
            ~PSKStaticHdlr();

            static PSKServerWallet *getServerWallet(void *sslh);
            static PSKClientValue *getClientValue(void *sslh);
            bool setSSLHandler(SSL *sslh);

        private:
            PSKClientValue *m_pskClientValues;
            PSKServerWallet *m_pskServerValues;
            SSL *m_sslHandlerForPSK;

            static std::map<void *, PSKClientValue *> m_clientPSKBySSLHandlerMap;
            static std::map<void *, PSKServerWallet *> m_serverPSKBySSLHandlerMap;
            static std::mutex m_clientPSKBySSLHandlerMapMutex, m_serverPSKBySSLHandlerMapMutex;
        };

        TLSKeyParameters(bool *isServer);
        ~TLSKeyParameters();

        /**
         * @brief setPSK Set to use PSK via DHE-PSK-AES256-GCM-SHA384 instead of using PKI (As Server)
         */
        void setUsingPSK();
        /**
        * @brief addPSKToServer Add Key to the server wallet to use in PSK and call setUsingPSK()
        * @param _psk Pre-shared key
        * @param clientIdentity client identity string (you can load multiple clients PSK's)
        */
        void addPSKToServer(const std::string &clientIdentity, const std::string &_psk);
        /**
        * @brief loadPSKAsClient Set to use PSK via DHE-PSK-AES256-GCM-SHA384 instead of using PKI (As Server) and add the key
        * @param _psk Pre-shared key
        * @param clientIdentity client identity string
        */
        void loadPSKAsClient(const std::string &clientIdentity, const std::string &_psk);

        /**
        * @brief linkPSKWithTLSHandle Internal function to set the SSL handler (as ID)
        * @param sslh
        * @return true if psk was configured.
        */
        bool linkPSKWithTLSHandle(SSL *sslh);

        /**
         * @brief prepareTLSContext Prepare the TLS context with the loaded keys
         * @param sslContext TLS context
         * @return true if succeed, false otherwise
         */
        bool initTLSKeys(SSL_CTX *ctx, SSL *sslh, std::list<std::string> *keyErrors);
        // Private Key From PEM File:
        /**
         * @brief loadPrivateKeyFromPEMFileEP Load Private Key From encrypted PEM File
         * @param filePath file path
         * @param pass null terminated password
         * @return true if loaded OK, false otherwise
         */
        bool loadPrivateKeyFromPEMFileEP(const char *filePath, const char *pass)
        {
            return loadPrivateKeyFromPEMFile(filePath, nullptr, (void *) pass);
        }
        /**
         * @brief loadPrivateKeyFromPEMFile Load Private Key From PEM File
         * @param filePath file path
         * @return true if file was loaded ok, false otherwise
         */
        bool loadPrivateKeyFromPEMFile(const char *filePath, pem_password_cb *cbPass = nullptr, void *u = nullptr);

        // Public Key From PEM File:
        /**
         * @brief loadPublicKeyFromPEMFileEP Load Public Key From encrypted PEM File
         * @param filePath file path
         * @param pass null terminated password
         * @return true if loaded OK, false otherwise
         */
        bool loadPublicKeyFromPEMFileEP(const char *filePath, const char *pass) { return loadPublicKeyFromPEMFile(filePath, nullptr, (void *) pass); }
        /**
         * @brief loadPublicKeyFromPEMFile Load Public Key From PEM File
         * @param filePath file path
         * @return true if file was loaded OK, false otherwise
         */
        bool loadPublicKeyFromPEMFile(const char *filePath, pem_password_cb *cbPass = nullptr, void *u = nullptr);

        // Private Key From PEM Memory
        /**
         * @brief loadPrivateKeyFromPEMMemoryEP Load Private Key from PEM formatted encrypted memory (null-terminated)
         * @note don't forget to clean the privKeyPEMData memory after use.
         * @param privKeyPEMData Private Key Memory (the memory will be destroyed on this function)
         * @param pass null terminated password
         * @return true if loaded OK, false otherwise
         */
        bool loadPrivateKeyFromPEMMemoryEP(const char *privKeyPEMData, const char *pass)
        {
            return loadPrivateKeyFromPEMMemory(privKeyPEMData, nullptr, (void *) pass);
        }
        /**
         * @brief loadPrivateKeyFromPEMMemory Load Private Key from PEM formatted memory (null-terminated)
         * @note don't forget to clean the privKeyPEMData memory after use.
         * @param privKeyPEMData Private Key Memory (the memory will be destroyed on this function)
         * @param u if cbPass is nullptr, u can be the password in null terminated format, otherwise u will be passed to the callback
         * @return true if loaded OK, false otherwise
         */
        bool loadPrivateKeyFromPEMMemory(const char *privKeyPEMData, pem_password_cb *cbPass = nullptr, void *u = nullptr);

        // Public Key From PEM Memory
        /**
         * @brief loadPublicKeyFromPEMMemoryEP Load Public Key from encrypted PEM formatted memory (null-terminated)
         * @param pubKeyPEMData Public Key Encrypted Memory (the memory will be destroyed on this function)
         * @param pass null terminated password
         * @return true if loaded OK, false otherwise
         */
        bool loadPublicKeyFromPEMMemoryEP(const char *privKeyPEMData, const char *pass)
        {
            return loadPublicKeyFromPEMMemory(privKeyPEMData, nullptr, (void *) pass);
        }
        /**
         * @brief loadPublicKeyFromPEMMemory Load Public Key from PEM formatted memory (null-terminated)
         * @param pubKeyPEMData Public Key Memory (the memory will be destroyed on this function)
         * @return true if loaded OK, false otherwise
         */
        bool loadPublicKeyFromPEMMemory(const char *pubKeyPEMData, pem_password_cb *cbPass = nullptr, void *u = nullptr);
        /**
         * @brief getTLSCipherList Get the TLS Cipher List
         * @return configured TLS cipher list (openssl format)
         */
        const std::string &getTLSCipherList() const;
        /**
         * @brief setTLSCipherList Set the TLS Cipher List
         * @param newSTLSCipherList  TLS cipher list string (openssl format)
         */
        void setTLSCipherList(const std::string &newSTLSCipherList);
        /**
         * @brief getCAPath Get Certificate Authority File Path
         * @return Certificate Authority File Path
         */
        const std::string &getCAPath() const;
        /**
         * @brief loadCAFromPEMFile Load Certificate Authority from PEM formatted file (the file must survive until the end of the connection)
         * @param newSTLSCertificateAuthorityPath Certificate Authority File Path
         * @return true if file exist
         */
        bool loadCAFromPEMFile(const std::string &newSTLSCertificateAuthorityPath);
        /**
         * @brief loadCAFromPEMMemory Load Certificate Authority from PEM formatted memory (null-terminated)
         * @param newSTLSCertificateAuthorityPath Certificate Authority File Path
         * @param suffix null terminated CA suffix string (nullptr = use random value)
         * @return true if temporary file was created
         */
        bool loadCAFromPEMMemory(const char *caCrtPEMData, const char *suffix = nullptr);
        /**
         * @brief getVerifyMaxDepth Get Verify Max Depth
         * @return max number of intermediary CA's
         */
        int getVerifyMaxDepth() const;
        /**
         * @brief setVerifyMaxDepth Set Verify Max Depth
         * @param newIVerifyMaxDepth max number of intermediary CA's
         */
        void setVerifyMaxDepth(int newIVerifyMaxDepth);

        // Callbacks:
        static unsigned int cbPSKServer(SSL *ssl, const char *identity, unsigned char *psk, unsigned int max_psk_len);
        static unsigned int cbPSKClient(
            SSL *ssl, const char *hint, char *identity, unsigned int max_identity_len, unsigned char *psk, unsigned int max_psk_len);

        /**
         * @brief getPSKClientValue
         * @return
         */
        PSKClientValue *getPSKClientValue();
        /**
         * @brief getPSKServerWallet
         * @return
         */
        PSKServerWallet *getPSKServerWallet();

        /**
         * @brief getSecurityLevel Get OpenSSL Security Level
         * @return Locally Configured Security Level
         */
        int getSecurityLevel() const;
        /**
         * @brief setSecurityLevel Set the locally configured security level for openssl (documentation taken from https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_security_level.html)
         * @param newSecurityLevel
         *
         * Level 0
         *      Everything is permitted. This retains compatibility with previous versions of OpenSSL.
         *
         * Level 1
         *      The security level corresponds to a minimum of 80 bits of security. Any parameters offering below 80 bits of security are excluded.
         *      As a result RSA, DSA and DH keys shorter than 1024 bits and ECC keys shorter than 160 bits are prohibited. All export cipher suites
         *      are prohibited since they all offer less than 80 bits of security. SSL version 2 is prohibited. Any cipher suite using MD5 for the MAC
         *      is also prohibited.
         *
         * Level 2
         *      Security level set to 112 bits of security. As a result RSA, DSA and DH keys shorter than 2048 bits and ECC keys shorter than 224 bits
         *      are prohibited. In addition to the level 1 exclusions any cipher suite using RC4 is also prohibited. SSL version 3 is also not allowed.
         *      Compression is disabled.
         *
         * Level 3
         *      Security level set to 128 bits of security. As a result RSA, DSA and DH keys shorter than 3072 bits and ECC keys shorter than 256 bits
         *      are prohibited. In addition to the level 2 exclusions cipher suites not offering forward secrecy are prohibited. TLS versions below 1.1
         *      are not permitted. Session tickets are disabled.
         *
         * Level 4
         *      Security level set to 192 bits of security. As a result RSA, DSA and DH keys shorter than 7680 bits and ECC keys shorter than 384 bits
         *      are prohibited. Cipher suites using SHA1 for the MAC are prohibited. TLS versions below 1.2 are not permitted.
         *
         * Level 5
         *      Security level set to 256 bits of security. As a result RSA, DSA and DH keys shorter than 15360 bits and ECC keys shorter than 512 bits
         *      are prohibited.
         */
        void setSecurityLevel(int newSecurityLevel);
        /**
         * @brief getMinProtocolVersion Get Minimum TLS Protocol Version
         * @return Minimum TLS Protocol Version (in openssl define version eg. TLS1_2_VERSION)
         */
        int getMinProtocolVersion() const;
        /**
         * @brief setMinProtocolVersion Set the Minimum TLS Protocol Version
         * @param newMinProtocolVersion TLS Protocol Version (in openssl define version eg. TLS1_2_VERSION)
         */
        void setMinProtocolVersion(int newMinProtocolVersion);
        /**
         * @brief getMaxProtocolVersion Get the Max TLS Protocol Version
         * @return TLS Max Protocol Version (in openssl define version eg. TLS1_2_VERSION)
         */
        int getMaxProtocolVersion() const;
        /**
         * @brief setMaxProtocolVersion Set the Max TLS Protocol Version
         * @param newMaxProtocolVersion  TLS Max Protocol Version (in openssl define version eg. TLS1_3_VERSION)
         */
        void setMaxProtocolVersion(int newMaxProtocolVersion);
        /**
         * @brief getUseSystemCertificates Get if using default verify locations
         * @return true for using default verify locations
         */
        bool getUseSystemCertificates() const;
        /**
         * @brief setUseSystemCertificates Set to use default verify locations (default: false)
         * @param newBVerifyDefaultLocations true to use default verify locations
         */
        void setUseSystemCertificates(bool newBVerifyDefaultLocations);
        /**
         * @brief getTLSSharedGroups Get the locally configured TLSv1.3 Shared Groups
         * @return locally configured groups by setTLSSharedGroups
         */
        const std::string &getTLSSharedGroups() const;
        /**
         * @brief setTLSSharedGroups Set TLSv1.3 Shared Groups (P-256, P-384, P-521, X25519, X448, ffdhe2048, ffdhe3072, ffdhe4096, ffdhe6144, ffdhe8192)
         * @param newSTLSSharedGroups groups separated by :
         */
        void setTLSSharedGroups(const std::string &newSTLSSharedGroups);
        /**
         * @brief getSTLSCipherSuites Get locally configured TLSv1.3 Cipher Suites
         * @return string with ciphersuites (ref. https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_cipher_list.html)
         */
        const std::string &getSTLSCipherSuites() const;
        /**
         * @brief setSTLSCipherSuites Set locally configured TLSv1.3 cipher suites
         * @param newSTLSCipherSuites string with ciphersuites (ref. https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_cipher_list.html)
         */
        void setSTLSCipherSuites(const std::string &newSTLSCipherSuites);

        bool getValidateServerHostname() const;
        void setValidateServerHostname(bool newValidateServerHostname);

    private:
        /**
         * @brief get_dh4096 Get the default configured Diffie Hellman 4096bit parameter
         * @return diffie hellman key.
         */
        static DH *get_dh4096(void);

        // SSL Key Parameters
        DH *m_dhParameter = nullptr;

        // Default: No private/public keys
        EVP_PKEY *m_privateKey = nullptr;
        X509 *m_publicKey = nullptr;

        // Other TLS protocols are insecure (we use from 1.2)...
        int m_minProtocolVersion = TLS1_2_VERSION;
        // Defined in the constructor
        int m_maxProtocolVersion;

        // TLS Security Level:
        int m_securityLevel = 2;

        PSKClientValue m_pskClientValues;
        PSKServerWallet m_pskServerValues;
        PSKStaticHdlr m_pskStaticHandler;

        std::string m_TLSCertificateAuthorityPath, m_TLSCertificateAuthorityMemory;

        /**
         * @brief sTLSCipherSuites TLSv1.3 cipher suites...
         */
        std::string m_TLSCipherSuites;
        /**
         * @brief sTLSCipherList TLSv1.2 (or lesser) Cipher List
         */
        std::string m_TLSCipherList = "DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:"
                                      "ECDHE-RSA-CHACHA20-POLY1305";

        /**
         * @brief sTLSSharedGroups TLSv1.3 (or greater) Shared groups List
         */
        std::string m_TLSSharedGroups;

        /**
         * @brief iVerifyMaxDepth Verify Max Depth value (-1: use default)
         */
        int m_maxVerifyDepth = -1;

        // Parent objects:
        bool *m_isServer;

        bool m_useSystemCertificates = false;
        bool m_validateServerHostname = false;
    };

    TLSKeyParameters m_keys;

    enum eCertValidationOptions
    {
        CERT_X509_VALIDATE,
        CERT_X509_CHECKANDPASS,
        CERT_X509_NOVALIDATE
    };

    /**
     * Class constructor.
     */
    Socket_TLS();
    /**
     * Class destructor.
     */
    virtual ~Socket_TLS();

    /**
     * TLS server function for protocol initialization , it runs in blocking mode and should be called apart to avoid tcp accept while block
     * @return returns true if was properly initialized.
     */
    bool postAcceptSubInitialization() override;
    /**
     * Accept a new TCP connection on a listening socket. The connection should be initialized with "postAcceptSubInitialization"
     * @return returns a socket with the new established tcp connection.
     */
    Socket_Stream_Base *acceptConnection() override;
    /**
     * Read a data block from the TLS socket
     * Receive the data block in only one command (without chunks).
     * note that this haves some limitations. some systems can only receive 4k at time.
     * You may want to manage the chunks by yourself.
     * @param data data block.
     * @param datalen data length in bytes
     * @return return the number of bytes read by the socket, zero for end of file and -1 for error.
     */
    virtual int partialRead(void *data, const uint32_t &datalen) override;
    /**
     * Write a data block to the TLS socket
     * note that this haves some limitations. some systems can only send 4k at time.
     * You may want to manage the chunks by yourself.
     * @param data data block.
     * @param datalen data length in bytes
     * @return return the number of bytes read by the socket, zero for end of file and -1 for error.
     */
    virtual int partialWrite(const void *data, const uint32_t &datalen) override;

    /////////////////////////
    // SSL functions:
    /**
     * Call this when program starts.
     */
    static void prepareTLS();
    /**
     * @brief setTLSParent Internal function to set the TLS parent object (listening socket to accepted socket)
     * @param parent listening socket
     */
    void setTLSParent(Socket_TLS *parent);

    // Setters:
    /**
     * @brief setServer Mark this socket as server (useful to determine if works as server or client)
     * @param value true for server.
     */
    void setServerMode(bool value);
    /**
     * @brief setTLSCipherList Set the TLS cipher list
     * @param newTLSCipherList cipher list to configure before establishing the connection
     */
    void setTLSCipherList(const std::string &newTLSCipherList);

    // Getters:
    /**
     * @brief getTLSConnectionCipherName Get current cipher used by current connection
     * @return current cipher
     */
    std::string getTLSConnectionCipherName();
    /**
     * @brief getTLSConnectionCipherBits Get current cipher bits of current connection
     * @return bits used
     */
    sCipherBits getTLSConnectionCipherBits();
    /**
     * @brief getTLSConnectionProtocolVersion Get protocol version name of current connection
     * @return cipher version string
     */
    std::string getTLSConnectionProtocolVersion();
    /**
     * @brief getTLSAcceptInvalidServerCerts Get if accept invalid server certificates
     * @return true if accept (default false)
     */
    bool getTLSAcceptInvalidServerCerts() const;
    /**
     * @brief getTLSErrorsAndClear Get TLS/SSL Errors
     * @return List of SSL Errors.
     */
    std::list<std::string> getTLSErrorsAndClear();
    /**
     * @brief getTLSPeerCN Get TLS Peer Common Name for PKI or identity for PSK
     * @return string with the CN or identity
     */
    std::string getTLSPeerCN();
    /**
     * @brief getCertValidation Get if we are accepting Invalid Server Certificates
     * @return Validation Option (validate, not validate or validate but ignore)
     */
    eCertValidationOptions getCertValidation() const;
    /**
     * @brief setCertValidation Set if we are accepting Invalid Server Certificates
     * @param newCertValidation (validate, not validate or validate but ignore)
     */
    void setCertValidation(eCertValidationOptions newCertValidation);

    bool isServer() const;

    bool isUsingPSK();

    ////////////////////////////////////////////////////////////////////
    // Socket Overrides:
    int iShutdown(int mode) override;
    bool isSecure() override;

protected:
    /**
     * function for TLS client protocol initialization after the connection starts (client-mode)...
     * @return returns true if was properly initialized.
     */
    bool postConnectSubInitialization() override;

private:
    bool createTLSContext();
    void parseErrors();
    bool validateTLSConnection(const bool &usingPSK);

    Socket_TLS *m_tlsParentConnection;

    eCertValidationOptions m_certValidationOptions;
    SSL *m_sslHandler;
    SSL_CTX *m_sslContext;
    SSL_CTX *createServerSSLContext();
    SSL_CTX *createClientSSLContext();

    std::list<std::string> m_sslErrorList;

    bool m_isServer;
};
} // namespace Sockets
} // namespace Network
} // namespace Mantids29
