/**
 * Provides utility functions for working with JSON data.
 */
#ifndef JSON_H
#define JSON_H

#include <json/json.h>
#include <list>

// Define a shorthand for the Json::Value type
typedef Json::Value json;

// jsoncpp macros:
#define JSON_ASCSTRING(j,x,def) (j[x].isString()?j[x].asCString():def)
#define JSON_ASSTRING(j,x,def) (j[x].isString()?j[x].asString():def)
#define JSON_ASBOOL(j,x,def) (j[x].isBool()?j[x].asBool():def)
#define JSON_ASDOUBLE(j,x,def) (j[x].isDouble()?j[x].asDouble():def)
#define JSON_ASFLOAT(j,x,def) (j[x].isFloat()?j[x].asFloat():def)
#define JSON_ASINT(j,x,def) (j[x].isInt()?j[x].asInt():def)
#define JSON_ASINT64(j,x,def) (j[x].isInt64()?j[x].asInt64():def)
#define JSON_ASUINT(j,x,def) (j[x].isUInt()?j[x].asUInt():def)
#define JSON_ASUINT64(j,x,def) (j[x].isUInt64()?j[x].asUInt64():def)
#define JSON_ISARRAY(j,x) (j.isMember(x) && j[x].isArray())

namespace Mantids29 { namespace Helpers {

    /**
     * Converts a JSON value to a string.
     *
     * @param value The JSON value to convert.
     *
     * @return The JSON value as a string.
     */
    std::string jsonToString(const json &value);

    /**
     * Converts a JSON array to a list of strings.
     *
     * @param value The JSON array to convert.
     * @param sub The name of the sub-element to extract from each array element (optional).
     *
     * @return A list of strings containing the elements of the JSON array.
     */
    std::list<std::string> jsonToStringList(const json &value, const std::string &sub = "");

    /**
     * A replacement for the deprecated Json::Reader class.
     *
     * Note: This class is named JSONReader2 to avoid conflicts with the existing JSONReader class in json.h.
     */
    class JSONReader2 {
    public:
        /**
         * Constructs a new JSON reader.
         */
        JSONReader2();

        /**
         * Destroys the JSON reader.
         */
        ~JSONReader2();

        /**
         * Parses a JSON document into a JSON value.
         *
         * @param document The JSON document to parse.
         * @param root A reference to a Json::Value object that will contain the parsed JSON value.
         *
         * @return True if the document was parsed successfully, false otherwise.
         */
        bool parse(const std::string &document, json &root);

        /**
         * Gets the error messages generated during the last parsing operation.
         *
         * @return A string containing the error messages.
         */
        std::string getFormattedErrorMessages();

    private:
        Json::CharReader *m_reader;
        std::string m_errors;
    };

}}; // End namespace Mantids29::Helpers

#endif // JSON_H
