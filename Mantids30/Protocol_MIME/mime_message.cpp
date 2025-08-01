#include "mime_message.h"
#include "Mantids30/Memory/parser.h"

#include <Mantids30/Helpers/crypto.h>
#include <Mantids30/Helpers/random.h>
#include <Mantids30/Memory/b_chunks.h>
#include <Mantids30/Memory/streamableobject.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <memory>
#include <optional>

using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Network::Protocols::MIME;
using namespace Mantids30;

MIME_Message::MIME_Message(std::shared_ptr<Memory::Streams::StreamableObject> value) : Memory::Streams::Parser(value, false)
{
    m_maxVarContentSize = 32*KB_MULT;

    m_currentParser = &m_subFirstBoundary;

    renewCurrentPart();
    setMultiPartBoundary(Helpers::Random::createRandomString(64));

}

std::shared_ptr<MIME_Message> MIME_Message::create(
    std::shared_ptr<StreamableObject> value)
{
    auto x = std::shared_ptr<MIME_Message>(new MIME_Message(value));
    x->initSubParser(&x->m_subFirstBoundary);
    x->initSubParser(&x->m_subEndPBoundary);
    x->m_initialized = x->initProtocol();
    return x;
}


bool MIME_Message::streamTo(Memory::Streams::StreamableObject * out)
{
    Memory::Streams::WriteStatus cur;
    // first boundary:
    out->writeString("--" + m_multiPartBoundary);
    if (!out->writeStatus.succeed)
        return false;

    for (std::shared_ptr<MIME_PartMessage>  i : m_allParts)
    {
        out->writeString("\r\n");

        // CHECK
        if (!out->writeStatus.succeed)
            return false;

        i->getContent()->initElemParser(out,true);

        // CHECK
        if (!out->writeStatus.succeed)
            return false;

        i->getHeader()->initElemParser(out,true);

        // CHECK
        if (!out->writeStatus.succeed)
            return false;

        if (!i->streamToSubParsers())
            return false;

        // Reset init element parsers...
        i->getContent()->initElemParser(nullptr,true);
        i->getHeader()->initElemParser(nullptr,true);


        out->writeString("--" + m_multiPartBoundary);

        // CHECK
        if (!out->writeStatus.succeed)
            return false;
    }

    out->writeString("--\r\n");
    // CHECK
    return out->writeStatus.succeed;
}

void MIME_Message::clear()
{
    m_currentPart = nullptr;
    m_allParts.clear();
    m_partsByName.clear();
}

uint32_t MIME_Message::varCount(const std::string &varName)
{
    uint32_t ix=0;
    auto range = m_partsByName.equal_range(boost::to_upper_copy(varName));
    for (auto i = range.first; i != range.second; ++i) ix++;
    return ix;
}

std::shared_ptr<Memory::Streams::StreamableObject> MIME_Message::getValue(const std::string &varName)
{
    auto range = m_partsByName.equal_range(boost::to_upper_copy(varName));
    for (auto i = range.first; i != range.second; ++i)
        return (((std::shared_ptr<MIME_PartMessage> )i->second)->getContent()->getContentContainer());
    return nullptr;
}

std::list<std::shared_ptr<Memory::Streams::StreamableObject> > MIME_Message::getValues(const std::string &varName)
{
    std::list<std::shared_ptr<Memory::Streams::StreamableObject> > values;
    auto range = m_partsByName.equal_range(boost::to_upper_copy(varName));
    for (auto i = range.first; i != range.second; ++i)
        values.push_back(((std::shared_ptr<MIME_PartMessage> )i->second)->getContent()->getContentContainer());
    return values;
}

std::set<std::string> MIME_Message::getKeysList()
{
    std::set<std::string> r;
    for ( const auto & i : m_partsByName ) r.insert(i.first);
    return r;
}

bool MIME_Message::isEmpty()
{
    return m_partsByName.empty();
}

void MIME_Message::iSetMaxVarContentSize()
{
    m_currentPart->getContent()->setMaxContentSize(m_maxVarContentSize);
}

void MIME_Message::renewCurrentPart()
{
    m_currentPart = std::make_shared<MIME_PartMessage>();

    initSubParser(m_currentPart->getContent());
    initSubParser(m_currentPart->getHeader());

    m_currentPart->getContent()->setBoundary( m_multiPartBoundary );
    m_currentPart->getContent()->setMaxContentSize(m_maxVarContentSize);

    // Header:
    m_currentPart->getHeader()->setMaxOptions(m_maxHeaderOptionsCount);
    m_currentPart->getHeader()->setMaxOptionSize(m_maxHeaderOptionSize);

    // Sub Header:
    m_currentPart->getHeader()->setMaxSubOptionCount(m_maxHeaderSubOptionsCount);
    m_currentPart->getHeader()->setMaxSubOptionSize(m_maxHeaderSubOptionsSize);
}

void MIME_Message::setCallbackOnHeaderReady(const sMIMECallback &newCallbackOnHeaderReady)
{
    m_onHeaderReady = newCallbackOnHeaderReady;
}

void MIME_Message::setCallbackOnContentReady(const sMIMECallback &newCallbackOnContentReady)
{
    m_onContentReady = newCallbackOnContentReady;
}

size_t MIME_Message::getMaxHeaderOptionSize() const
{
    return m_maxHeaderOptionSize;
}

void MIME_Message::setMaxHeaderOptionSize(const size_t &value)
{
    m_maxHeaderOptionSize = value;
    m_currentPart->getHeader()->setMaxOptionSize(m_maxHeaderOptionSize);
}

size_t MIME_Message::getMaxHeaderOptionsCount() const
{
    return m_maxHeaderOptionsCount;
}

void MIME_Message::setMaxHeaderOptionsCount(const size_t &value)
{
    m_maxHeaderOptionsCount = value;
    m_currentPart->getHeader()->setMaxOptions(m_maxHeaderOptionsCount);
}

size_t MIME_Message::getMaxHeaderSubOptionsSize() const
{
    return m_maxHeaderSubOptionsSize;
}

void MIME_Message::setMaxHeaderSubOptionsSize(const size_t &value)
{
    m_maxHeaderSubOptionsSize = value;
    m_currentPart->getHeader()->setMaxSubOptionSize(m_maxHeaderSubOptionsSize);
}

size_t MIME_Message::getMaxHeaderSubOptionsCount() const
{
    return m_maxHeaderSubOptionsCount;
}

void MIME_Message::setMaxHeaderSubOptionsCount(const size_t &value)
{
    m_maxHeaderSubOptionsCount = value;
    m_currentPart->getHeader()->setMaxSubOptionCount(m_maxHeaderSubOptionsCount);
}

size_t MIME_Message::getMaxParts() const
{
    return m_maxNumberOfParts;
}

void MIME_Message::setMaxParts(const size_t &value)
{
    m_maxNumberOfParts = value;
}

bool MIME_Message::changeToNextParser()
{
    switch (m_currentState)
    {
    case MP_STATE_FIRST_BOUNDARY:
    {
        // FIRST_BOUNDARY IS READY
        m_currentState = MP_STATE_HEADERS;
        m_currentParser = m_currentPart->getHeader();
    }break;
    case MP_STATE_ENDPOINT:
    {
        // ENDPOINT IS READY
        if (m_subEndPBoundary.getStatus() == ENDP_STAT_CONTINUE)
        {
            m_currentState = MP_STATE_HEADERS;
            m_currentParser = m_currentPart->getHeader();
        }
        else
            m_currentParser = nullptr;
    }break;
    case MP_STATE_HEADERS:
    {
        // HEADERS ARE READY
        // Callback:
        m_onHeaderReady.call(getMultiPartMessageName(m_currentPart),m_currentPart);
        // GOTO CONTENT:
        m_currentState = MP_STATE_CONTENT;
        m_currentParser = m_currentPart->getContent();
    }break;
    case MP_STATE_CONTENT:
    {
        // CONTENT IS READY
        // Callback:
        m_onContentReady.call(getMultiPartMessageName(m_currentPart),m_currentPart);

        // Put current Part.
        addMultiPartMessage(m_currentPart);
        // new current part definition.
        renewCurrentPart();
        // Check if the max parts target is reached.
        if (m_allParts.size()==m_maxNumberOfParts)
            m_currentParser = nullptr; // End here.
        else
        {
            // Goto boundary.
            m_currentState = MP_STATE_ENDPOINT;
            m_subEndPBoundary.reset();
            m_currentParser = &m_subEndPBoundary;
        }
    }break;
    default:
    {
    }break;
    }
    return true;
}

void MIME_Message::addMultiPartMessage(std::shared_ptr<MIME_PartMessage> part)
{
    m_allParts.push_back(part);
    // Insert by name:
    std::string varName = getMultiPartMessageName(part);
    if (varName!="")
    {
        m_partsByName.insert(std::pair<std::string,std::shared_ptr<MIME_PartMessage>>(boost::to_upper_copy(varName),part));
    }
}

std::string MIME_Message::getMultiPartMessageName(std::shared_ptr<MIME_PartMessage> part)
{
    std::shared_ptr<MIME_HeaderOption> opt = part->getHeader()->getOptionByName("content-disposition");
    if (opt)
    {
        return opt->getSubVar("name");
    }
    return "";
}

std::list<std::shared_ptr<MIME_PartMessage> > MIME_Message::getMultiPartMessagesByName(const std::string &varName)
{
    std::list<std::shared_ptr<MIME_PartMessage> > values;
    auto range = m_partsByName.equal_range(boost::to_upper_copy(varName));
    for (auto i = range.first; i != range.second; ++i) values.push_back(i->second);
    return values;
}

std::shared_ptr<MIME_PartMessage> MIME_Message::getFirstMessageByName(const std::string &varName)
{
    if (m_partsByName.find(boost::to_upper_copy(varName)) == m_partsByName.end()) 
        return nullptr;
    return m_partsByName.find(boost::to_upper_copy(varName))->second;
}

bool MIME_Message::addStringVar(const std::string &key, const std::string &value)
{
    // TODO: check max size?
    if (    m_currentPart->getHeader()->add("Content-Disposition", "form-data") &&
        m_currentPart->getHeader()->add("name", key)
        )
    {
        auto contentBuffer = std::make_shared<Memory::Containers::B_Chunks>();
        contentBuffer->append(value.c_str(), value.size());
        m_currentPart->getContent()->replaceContentContainer(contentBuffer);

        addMultiPartMessage(m_currentPart);
        renewCurrentPart();
    }
    else
    {
        return false;
    }

    return true;
}

bool MIME_Message::addReferecedFileVar(const std::string &varName, const std::string &filePath)
{
    // TODO: check max size?
    auto fContainer = std::make_shared<Memory::Containers::B_MMAP>();
    if (!fContainer->referenceFile(filePath,true,false))
    {
        return false;
    }
    return addStreamableObjectContainer(varName,fContainer);
}

bool MIME_Message::addVar(
    const std::string &varName, std::shared_ptr<Memory::Containers::B_Chunks> data)
{
    return addStreamableObjectContainer(varName,data);
}

bool MIME_Message::addStreamableObjectContainer(
    const std::string &varName, std::shared_ptr<StreamableObject> obj)
{
    if (m_currentPart->getHeader()->add("Content-Disposition", "form-data") &&
        m_currentPart->getHeader()->add("name", varName) )
    {
        m_currentPart->getContent()->replaceContentContainer(obj);
        addMultiPartMessage(m_currentPart);
        renewCurrentPart();
    }
    else
    {
        return false;
    }
    return true;
}

std::string MIME_Message::getMultiPartType() const
{
    return m_multiPartType;
}

void MIME_Message::setMultiPartType(const std::string &value)
{
    m_multiPartType = value;
}

std::string MIME_Message::getMultiPartBoundary() const
{
    return m_multiPartBoundary;
}

void MIME_Message::setMultiPartBoundary(const std::string &value)
{
    m_multiPartBoundary = value;

    m_subFirstBoundary.setBoundary(m_multiPartBoundary);

    if (m_currentPart)
        m_currentPart->getContent()->setBoundary(m_multiPartBoundary);
}

bool MIME_Message::initProtocol()
{
    return true;
}

void MIME_Message::endProtocol()
{

}
