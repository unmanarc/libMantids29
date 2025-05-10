#include "streams_cryptochallenge.h"
#include <Mantids30/Helpers/random.h>
#include <Mantids30/Helpers/crypto.h>

using namespace Mantids30;
using namespace Mantids30::Network::Sockets::NetStreams;

CryptoChallenge::CryptoChallenge(std::shared_ptr<Sockets::Socket_Stream> socket)
{
    this->m_socket = socket;
}

std::pair<bool, bool> CryptoChallenge::mutualChallengeResponseSHA256Auth(const std::string &sharedKey, bool server)
{
    bool readOK;
    std::string sLocalRandomValue = Helpers::Random::createRandomString(64), sRemoteRandomValue;

    // RND strings filled up.
    if (!m_socket->writeStringEx<uint8_t>(sLocalRandomValue))
        return std::make_pair(false,false);
    sRemoteRandomValue = m_socket->readStringEx<uint8_t>(&readOK);
    if (!readOK)
        return std::make_pair(false,false);

    if (sRemoteRandomValue.size()!=64)
        return std::make_pair(false,false);

    /////////////////////////////////////////////////////
    // Up to this positions, random values are in sync.

    // Send our guess as client:
    if (!server)
    {
        // The client should expose the sharedKey (hashed+randomly salted), so please use a truly random sharedKey to prevent key cracking
        if (!m_socket->writeStringEx<uint8_t>(Helpers::Crypto::calcSHA256(sharedKey+sRemoteRandomValue+sLocalRandomValue)))
            return std::make_pair(false,false);
    }

    // Read the remote guess
    std::string sRemoteChallenge = m_socket->readStringEx<uint8_t>(&readOK);
    if (!readOK)
        return std::make_pair(false,false);

    // Evaluate the remote Hash Calculation
    bool bRemoteChallengeOK = (sRemoteChallenge == Helpers::Crypto::calcSHA256(sharedKey+sLocalRandomValue+sRemoteRandomValue));

    // If we are the server and the authentication succeed, send our challenge
    if (server)
    {
        // The server won't expose the sharedKey if the client does not meet the challenge itself.
        if (!m_socket->writeStringEx<uint8_t>(Helpers::Crypto::calcSHA256((bRemoteChallengeOK?sharedKey:"")+sRemoteRandomValue+sLocalRandomValue)))
            return std::make_pair(false,false);
    }

    m_socket->writeU<uint8_t>(bRemoteChallengeOK?1:0);
    bool bLocalChallengeOK  = m_socket->readU<uint8_t>() == 1;

    return std::make_pair(bRemoteChallengeOK,bLocalChallengeOK);
}
