#include "http_server.hh"

#include <vector>

#include <sys/stat.h>

#include <fstream>
#include <sstream>

#include <dirent.h>
#include <time.h>

vector<string> split(const string &s, char delim)
{
  vector<string> elems;

  stringstream ss(s);
  string item;

  while (getline(ss, item, delim))
  {
    if (!item.empty())
      elems.push_back(item);
  }

  return elems;
}

HTTP_Request::HTTP_Request(string request)
{
  vector<string> lines = split(request, '\n');
  vector<string> first_line = split(lines[0], ' ');

  this->HTTP_version = "1.0"; // We'll be using 1.0 irrespective of the request

  /*
   TODO : extract the request method and URL from first_line here
  */
  this->method = first_line[0];
  this->url = first_line[1];

  if (this->method != "GET")
  {
    cerr << "Method '" << this->method << "' not supported" << endl;
    exit(1);
  }
}

HTTP_Response *handle_request(string req)
{
  HTTP_Request *request = new HTTP_Request(req);

  HTTP_Response *response = new HTTP_Response();

  string url = string("html_files") + request->url;

  response->HTTP_version = "1.0";

  struct stat sb;

  if (stat(url.c_str(), &sb) == 0) // requested path exists
  {
    response->status_code = "200";
    response->status_text = "OK";
    response->content_type = "text/html";

    string body;

    if (S_ISDIR(sb.st_mode)) {
      /*
      In this case, requested path is a directory.
      TODO : find the index.html file in that directory (modify the url
      accordingly)
      */
      if(url[url.size()-1] == '/')
        url += "index.html";
      else
        url += "/index.html";
    }
    const char *charUrl = url.c_str();
    /*
    TODO : open the file and read its contents
    */
    FILE *fileptr = fopen(charUrl, "r");
    struct stat si;
    stat(url.c_str(), &si);
    char buffer[si.st_size+1];
    int x = fread(buffer, si.st_size,1,fileptr);
    fclose(fileptr);
    /*
    TODO : set the remaining fields of response appropriately
    */
    response->body=buffer;
    response->content_length=to_string(si.st_size);
  }
  else
  {
    response->status_code = "404";

    /*
    TODO : set the remaining fields of response appropriately
    */

    string lines;

    response->status_text = "page not found";
    response->content_type = "text/html";

    lines = "<html><head><title>404 Not found</title><body>ERROR 404 Page Not Found</head></body></html>";
    int length = lines.length();

    response->body.append(lines);
    response->content_length = to_string(length);
  }

  delete request;

  return response;
}

string HTTP_Response::get_string()
{
  /*
  TODO : implement this function
  */

  string result = "";
  char buffer[1024];

  time_t timenow = time(0);

  struct tm time = *gmtime(&timenow);
  strftime(buffer, sizeof buffer, "%a, %d %b %Y %H:%M:%S %Z", &time);

  string HTTP_version = "HTTP/1.1 ";

  string status_text = this->status_text + "\n";
  string status_code = this->status_code + " ";

  string content_type = "\nContent-Type: " + this->content_type + "\n";
  string content_length = "Content-Length: " + this->content_length + "\r\n\n";

  result = HTTP_version + status_code + status_text + "Date:" + buffer + content_type + content_length + body;

  // cout << "result:\n" + result << endl;

  return result;
}