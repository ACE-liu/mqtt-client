#include "hfHttpClient.h"


using namespace std;


bool hfHttpClient::httpPostRequest(const std::string & url, 
                                   const std::map<std::string, std::string> & headers, 
                                   const std::string & body, std::string & rtn)
{
   if(url.empty())
    {
       return false;
    } 
    
    CURL *curl = NULL;  
    CURLcode code;  
    struct curl_slist* httpheaders = NULL;
    curl_global_init(CURL_GLOBAL_DEFAULT);  
     curl = curl_easy_init();    
	code = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());  

    code = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);  
 
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData); 
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&rtn); 

    
    for(auto it=headers.begin(); it!=headers.end(); ++it)
	{
		std::string tempStr = it->first + ": " + it->second;
		httpheaders = curl_slist_append(httpheaders, tempStr.data());
	}
     curl_easy_setopt(curl, CURLOPT_HTTPHEADER, httpheaders);
    
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2);  
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2); 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.length());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);   //https ssl验证
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    code = curl_easy_perform(curl);  
    curl_slist_free_all(httpheaders);
    curl_easy_cleanup(curl);       
    return code == CURLE_OK;     
}


size_t hfHttpClient::OnWriteData(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    std::string* str = static_cast<std::string*>(lpVoid);  
    if( NULL == str || NULL == buffer )  
    {  
        return -1;  
    }  
  
    char* pData = (char*)buffer;  
    str->append(pData, size * nmemb);  
    return nmemb; 
}