#pragma once

#include <map>

#include <Mantids30/Memory/vars.h>
#include <Mantids30/Memory/parser.h>
#include <memory>

#include "mime_partmessage.h"
#include "mime_sub_firstboundary.h"
#include "mime_sub_endpboundary.h"


namespace Mantids30 { namespace Network { namespace Protocols { namespace MIME {


class MIME_Message : public Memory::Abstract::Vars, public Memory::Streams::Parser
{
public:
    static std::shared_ptr<MIME_Message> create(std::shared_ptr<StreamableObject> value = nullptr);

    ~MIME_Message() override;

    bool streamTo(std::shared_ptr<Memory::Streams::StreamableObject>  out, Memory::Streams::StreamableObject::Status & wrStat) override;

    ////////////////////////////////////////////////////////////////////
    //        ------------- VARIABLE GET FUNCTIONS -------------
    ////////////////////////////////////////////////////////////////////
    /**
     * @brief varCount Get the number of variables with an specific name
     * @param varName variable name
     * @return number of elements
     */
    uint32_t varCount(const std::string & varName) override;
    /**
     * @brief getValue Get the memory container for an specific variable
     * @param varName Variable Name
     * @return Memory Container Base
     */
    std::shared_ptr<StreamableObject> getValue(const std::string & varName) override;
    /**
     * @brief getValues Get memory containers for an specific variable name (if one variable name contains multiple definitions)
     * @param varName Variable Name
     * @return list of memory containers
     */
    std::list<std::shared_ptr<StreamableObject>> getValues(const std::string & varName) override;
    /**
     * @brief getKeysList Get Variable Name List
     * @return Variable Name List
     */
    std::set<std::string> getKeysList() override;
    /**
     * @brief isEmpty Get if there is no defined variable
     * @return true if empty
     */
    bool isEmpty() override;
    // TODO: decode as BC pos reference (for file reading).
    /**
     * @brief getMultiPartMessagesByName Get Multipart Messages By Name
     * @param varName Variable Name
     * @return list of part message that match the given name
     */
    std::list<MIME_PartMessage *> getMultiPartMessagesByName(const std::string & varName);
    /**
     * @brief getFirstMessageByName Get the first message by name
     * @param varName Variable name
     * @return The first part message for a given variable name
     */
    MIME_PartMessage * getFirstMessageByName(const std::string & varName);


    ////////////////////////////////////////////////////////////////////
    //        ------------- VARIABLE SET FUNCTIONS -------------
    ////////////////////////////////////////////////////////////////////
    /**
     * @brief addString Add String Variable to the multipart message
     * @param varName Variable Name
     * @param varValue Variable Value
     * @return true if inserted, otherwise false (eg. limits)
     */
    bool addStringVar( const std::string & varName, const std::string & varValue );
    /**
     * @brief addReferecedFile Add File as Variable to the multipart message (will be read when transmitted)
     * @param varName Variable Name
     * @param filePath File Path
     * @return true if file exist and inserted, otherwise false (eg. limits)
     */
    bool addReferecedFileVar(const std::string & varName, const std::string & filePath );

    ////////////////////////////////////////////////////////////////////
    //        ------------- CALLBACKS OPTIONS -------------
    ////////////////////////////////////////////////////////////////////

    struct sMIMECallback
    {
        sMIMECallback( void (*callbackFunction)(void *, const std::string &, MIME_PartMessage *),  void * context)
        {
           this->callbackFunction = callbackFunction;
           this->context = context;
        }

        sMIMECallback()
        {
           callbackFunction = nullptr;
           context = nullptr;
        }

        void call(const std::string & partName, MIME_PartMessage *partMessage)
        {
            if (callbackFunction!=nullptr)
                callbackFunction(context,partName,partMessage);
        }

        void (*callbackFunction)(void *context, const std::string & partName, MIME_PartMessage *partMessage);
        void * context;
    };

    /**
     * @brief setCallbackOnContentReady Set callback when content is ready (this is useful to post-process an specific part, eg. move a tmp file)
     * @param newCallbackOnContentReady object with proper callback
     */
    void setCallbackOnContentReady(const sMIMECallback &newCallbackOnContentReady);
    /**
     * @brief setCallbackOnHeaderReady Set callback when header is ready (this is useful to redirect special content)
     * @param newCallbackOnHeaderReady object with proper callback
     */
    void setCallbackOnHeaderReady(const sMIMECallback &newCallbackOnHeaderReady);

    ////////////////////////////////////////////////////////////////////
    //        ------------- MULTIPART OPTIONS -------------
    ////////////////////////////////////////////////////////////////////
    /**
     * @brief getMultiPartType Get MultiPart Type (eg. multipart/form-data)
     * @return multipart type string
     */
    std::string getMultiPartType() const;
    /**
     * @brief setMultiPartType Set MultiPart Type (eg. multipart/form-data)
     * @param value multipart type string
     */
    void setMultiPartType(const std::string &value);
    /**
     * @brief getMultiPartBoundary Get the boundary for making the multipart message
     * @return boundary text
     */
    std::string getMultiPartBoundary() const;
    /**
     * @brief setMultiPartBoundary Set the boundary for making the multipart message
     * @param value boundary text
     */
    void setMultiPartBoundary(const std::string &value);

    ////////////////////////////////////////////////////////////////////
    //             ------------- SECURITY OPTIONS -------------
    ////////////////////////////////////////////////////////////////////
    /**
     * @brief getMaxParts Get Max number of parts allowed to be decoded on this container
     * @return Max number of parts
     */
    size_t getMaxParts() const;
    /**
     * @brief setMaxParts Set Max number of parts allowed to be decoded on this container
     * @param value max number of parts
     */
    void setMaxParts(const size_t &value);
    /**
     * @brief getMaxHeaderSubOptionsCount Get Max number of options that can be included in a part header
     * @return Max number of options that can be included in a part header
     */
    size_t getMaxHeaderSubOptionsCount() const;
    /**
     * @brief setMaxHeaderSubOptionsCount Set Max number of options that can be included in a part header
     * @param value Max number of options that can be included in a part header
     */
    void setMaxHeaderSubOptionsCount(const size_t &value);
    /**
     * @brief getMaxHeaderSubOptionsSize Get the max size that a option from a header can handle
     * @return max size that a option from a header can have
     */
    size_t getMaxHeaderSubOptionsSize() const;
    /**
     * @brief setMaxHeaderSubOptionsSize Set the max size that a option from a header can handle
     * @param value max size that a option from a header can have
     */
    void setMaxHeaderSubOptionsSize(const size_t &value);
    /**
     * @brief getMaxHeaderOptionsCount Get the max number of options that a part header can handle
     * @return max number of options that a part header can handle
     */
    size_t getMaxHeaderOptionsCount() const;
    /**
     * @brief setMaxHeaderOptionsCount Set the max number of options that a part header can handle
     * @param value max number of options that a part header can handle
     */
    void setMaxHeaderOptionsCount(const size_t &value);
    /**
     * @brief getMaxHeaderOptionSize Get the max header option size
     * @return max header option size
     */
    size_t getMaxHeaderOptionSize() const;
    /**
     * @brief setMaxHeaderOptionSize Set the max header option size
     * @param value max header option size
     */
    void setMaxHeaderOptionSize(const size_t &value);



protected:
    bool initProtocol() override;
    void endProtocol() override;
    bool changeToNextParser() override;

    ///////////////////////////////////////
    // Virtuals for Vars...  Security
    void iSetMaxVarContentSize() override;
    ///////////////////////////////////////

private:
    MIME_Message(std::shared_ptr<StreamableObject> value= nullptr);

    enum eMIME_VarStat
    {
        MP_STATE_FIRST_BOUNDARY,
        MP_STATE_ENDPOINT,
        MP_STATE_HEADERS,
        MP_STATE_CONTENT
    };

    void addMultiPartMessage(MIME_PartMessage * part);
    std::string getMultiPartMessageName(MIME_PartMessage * part);

    void renewCurrentPart();
    size_t m_maxNumberOfParts = 128;
    size_t m_maxHeaderSubOptionsCount = 16;
    size_t m_maxHeaderSubOptionsSize = 8 * KB_MULT;
    size_t m_maxHeaderOptionsCount = 64;
    size_t m_maxHeaderOptionSize = 8 * KB_MULT;

    // MIME Message Options:
    std::string m_multiPartType = "multipart/mixed";
    std::string m_multiPartBoundary;

    // Status:
    eMIME_VarStat m_currentState = MP_STATE_FIRST_BOUNDARY;


    // Message Parts:
    std::list<MIME_PartMessage *> m_allParts;
    std::multimap<std::string,MIME_PartMessage *> m_partsByName;
    MIME_PartMessage * m_currentPart = nullptr;
    MIME_Sub_FirstBoundary m_subFirstBoundary;
    MIME_Sub_EndPBoundary m_subEndPBoundary;

    // Callbacks:
    sMIMECallback m_onHeaderReady;
    sMIMECallback m_onContentReady;

};


}}}}

    

