#include "translation/LibreTranslate.h"

#include <vector>
#include <iostream> // Incluído para depuração de erros
#include <curl/curl.h>

size_t curl_writeback_fun(void* contents, size_t size, size_t nmemb, std::string *s) {
  size_t len = size * nmemb;
  s->append((char*)contents, len);
  return len;
}

std::string curl_post(std::string url, std::vector<std::string> headers, std::string post_fields){
  // Based on https://curl.se/libcurl/c/http-post.html
  // Daniel Stenberg, <daniel@haxx.se>, et al.

  std::string to_return;
  CURL *curl;
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());

    // ===== INÍCIO DA CORREÇÃO FINAL =====
    // Força o uso do HTTP/1.1 e desabilita o cabeçalho "Expect", aumentando a compatibilidade.
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    struct curl_slist *chunk = NULL;
    chunk = curl_slist_append(chunk, "Expect:");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    // ===== FIM DA CORREÇÃO FINAL =====

    struct curl_slist *headers_slist = NULL;
    if(headers.size() > 0){
        for(std::string header : headers){
          headers_slist = curl_slist_append(headers_slist, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_slist);
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writeback_fun);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &to_return);

    CURLcode res;
    res = curl_easy_perform(curl);

    if(res != CURLE_OK){
      // Limpa a lista de cabeçalhos mesmo se houver erro
      if (chunk) {
        curl_slist_free_all(chunk);
      }
      throw curl_easy_strerror(res);
    }

    if(headers_slist){
      curl_slist_free_all(headers_slist);
    }
    
    // Limpa a lista de cabeçalhos
    if (chunk) {
      curl_slist_free_all(chunk);
    }
 
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();

  return to_return;
}

json json_post(std::string url, json data){
  std::vector<std::string> headers;
  headers.push_back("Content-Type: application/json");

  std::string serialized_json = data.dump();
  
  std::string res = curl_post(url, headers, serialized_json);

  return json::parse(res);
}

LibreTranslateAPI::LibreTranslateAPI(std::string base_url)
  : base_url(base_url){}

json LibreTranslateAPI::translate(std::string q, std::string source, std::string target){
  std::string url = base_url + "translate";
  
  json req;
  req["q"] = q;
  req["source"] = source;
  req["target"] = target;

  json res = json_post(url, req);

  return res;
}

json LibreTranslateAPI::languages(){
  std::string url = base_url + "languages";
  
  json req;

  json res = json_post(url, req);

  return res;
}

json LibreTranslateAPI::detect(std::string q){
  std::string url = base_url + "detect";
  
  json req;
  req["q"] = q;

  json res = json_post(url, req);

  return res;
}

std::string LibreTranslateAPI::translateText(std::string q, std::string source, std::string target) {
    auto res = translate(q, source, target);
    if (res.contains("translatedText")) {
      std::string result = res["translatedText"];
      std::string cleaned_result;

      for (char c : result)
      {
          if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '.' || c == ',' || c == '!')
          {
              cleaned_result += c;
          }
      }
      return cleaned_result;
    }
    return "[erro]";
}