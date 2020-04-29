#ifndef  _HF_HTTP_CLIENT_H_
#define _HF_HTTP_CLIENT_H_

#include "curl/curl.h"
#include <string>
#include <map>

class hfHttpClient
{
public:
    static bool httpPostRequest(const std::string & url, const std::map<std::string, std::string> & headers, const std::string & body, std::string & rtn);

private:
    static size_t OnWriteData(void* buffer, size_t size, size_t nmemb, void* lpVoid);

};








#endif // ! _HF_HTTP_CLIENT_H_
