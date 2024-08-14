#include "lynx/http/http_request.h"

#include <cstring>
#include <sstream>

namespace lynx {

namespace detail {

static const char xdigit_chars[256] = {
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  1,  2,  3, 4, 5, 6, 7, 8, 9,  0,  0,
    0,  0,  0,  0, 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 10, 11, 12,
    13, 14, 15, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0,
};

std::string urlDecode(const std::string &str, bool space_as_plus = true) {
  std::string *ss = nullptr;
  const char *end = str.c_str() + str.length();
  for (const char *c = str.c_str(); c < end; ++c) {
    if (*c == '+' && space_as_plus) {
      if (!ss) {
        ss = new std::string;
        ss->append(str.c_str(), c - str.c_str());
      }
      ss->append(1, ' ');
    } else if (*c == '%' && (c + 2) < end && isxdigit(*(c + 1)) &&
               isxdigit(*(c + 2))) {
      if (!ss) {
        ss = new std::string;
        ss->append(str.c_str(), c - str.c_str());
      }
      ss->append(
          1, static_cast<char>(xdigit_chars[static_cast<int>(*(c + 1))] << 4 |
                               xdigit_chars[static_cast<int>(*(c + 2))]));
      c += 2;
    } else if (ss) {
      ss->append(1, *c);
    }
  }
  if (!ss) {
    return str;
  } else {
    std::string rt = *ss;
    delete ss;
    return rt;
  }
}

std::string trim(const std::string &str,
                 const std::string &delimit = " \t\r\n") {
  auto begin = str.find_first_not_of(delimit);
  if (begin == std::string::npos) {
    return "";
  }
  auto end = str.find_last_not_of(delimit);
  return str.substr(begin, end - begin + 1);
}

} // namespace detail

HttpMethod stringToHttpMethod(const std::string &m) {
#define XX(num, name, string)                                                  \
  if (strcmp(#string, m.c_str()) == 0) {                                       \
    return HttpMethod::name;                                                   \
  }
  HTTP_METHOD_MAP(XX);
#undef XX
  return HttpMethod::INVALID_METHOD;
}

HttpMethod charsToHttpMethod(const char *m) {
#define XX(num, name, string)                                                  \
  if (strncmp(#string, m, strlen(#string)) == 0) {                             \
    return HttpMethod::name;                                                   \
  }
  HTTP_METHOD_MAP(XX);
#undef XX
  return HttpMethod::INVALID_METHOD;
}

static const char *method_string[] = {
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

const char *methodToString(const HttpMethod &m) {
  uint32_t idx = static_cast<uint>(m);
  if (idx >= (sizeof(method_string) / sizeof(method_string[0]))) {
    return "unknown";
  }
  return method_string[idx];
}

HttpRequest::HttpRequest(uint8_t version, bool close)
    : method_(HttpMethod::GET), version_(version), close_(close),
      websocket_(false), parser_param_flag_(0), path_("/") {}

std::string HttpRequest::getHeader(const std::string &key,
                                   const std::string &def) const {
  auto it = headers_.find(key);
  return it == headers_.end() ? def : it->second;
}

std::string HttpRequest::getParam(const std::string &key,
                                  const std::string &def) const {
  auto it = params_.find(key);
  return it == params_.end() ? def : it->second;
}

std::string HttpRequest::getCookie(const std::string &key,
                                   const std::string &def) {
  auto it = cookies_.find(key);
  return it == cookies_.end() ? def : it->second;
}

void HttpRequest::setHeader(const std::string &key, const std::string &val) {
  headers_[key] = val;
}

void HttpRequest::setParam(const std::string &key, const std::string &val) {
  params_[key] = val;
}

void HttpRequest::setCookie(const std::string &key, const std::string &val) {
  cookies_[key] = val;
}

void HttpRequest::delHeader(const std::string &key) { headers_.erase(key); }
void HttpRequest::delParam(const std::string &key) { params_.erase(key); }
void HttpRequest::delCookie(const std::string &key) { cookies_.erase(key); }

bool HttpRequest::hasHeader(const std::string &key, std::string *val) {
  auto it = headers_.find(key);
  if (it == headers_.end()) {
    return false;
  }
  if (val) {
    *val = it->second;
  }
  return true;
}

bool HttpRequest::hasParam(const std::string &key, std::string *val) {
  auto it = params_.find(key);
  if (it == params_.end()) {
    return false;
  }
  if (val) {
    *val = it->second;
  }
  return true;
}

bool HttpRequest::hasCookie(const std::string &key, std::string *val) {
  auto it = cookies_.find(key);
  if (it == cookies_.end()) {
    return false;
  }
  if (val) {
    *val = it->second;
  }
  return true;
}

std::ostream &HttpRequest::dump(std::ostream &os) const {
  os << methodToString(method_) << " " << path_ << (query_.empty() ? "" : "?")
     << query_ << (fragment_.empty() ? "" : "#") << fragment_ << " HTTP/"
     << (static_cast<uint32_t>(version_ >> 4)) << "."
     << (static_cast<uint32_t>(version_ & 0x0F)) << "\r\n";
  if (!websocket_) {
    os << "connection: " << (close_ ? "close" : "keep-alive") << "\r\n";
  }
  for (auto &i : headers_) {
    if (!websocket_ && strcasecmp(i.first.c_str(), "connection") == 0) {
      continue;
    }
    os << i.first << ": " << i.second << "\r\n";
  }

  if (!body_.empty()) {
    os << "content-length: " << body_.size() << "\r\n\r\n" << body_;
  } else {
    os << "\r\n";
  }
  return os;
}

std::string HttpRequest::toString() const {
  std::stringstream ss;
  dump(ss);
  return ss.str();
}

#define PARSE_PARAM(str, m, flag, _trim_)                                      \
  size_t pos = 0;                                                              \
  do {                                                                         \
    size_t last = pos;                                                         \
    pos = str.find('=', pos);                                                  \
    if (pos == std::string::npos) {                                            \
      break;                                                                   \
    }                                                                          \
    size_t key = pos;                                                          \
    pos = str.find(flag, pos);                                                 \
    m.insert(std::make_pair(                                                   \
        _trim_(str.substr(last, key - last)),                                  \
        lynx::detail::urlDecode(str.substr(key + 1, pos - key - 1))));         \
    if (pos == std::string::npos) {                                            \
      break;                                                                   \
    }                                                                          \
    ++pos;                                                                     \
  } while (true);

void HttpRequest::initQueryParam() {
  if (parser_param_flag_ & 0x1) {
    return;
  }
  PARSE_PARAM(query_, params_, '&', );
  parser_param_flag_ |= 0x1;
}

void HttpRequest::initBodyParam() {
  if (parser_param_flag_ & 0x2) {
    return;
  }
  std::string content_type = getHeader("content-type");
  if (strcasestr(content_type.c_str(), "application/x-www-form-urlencoded") ==
      nullptr) {
    parser_param_flag_ |= 0x2;
    return;
  }
  PARSE_PARAM(body_, params_, '&', );
  parser_param_flag_ |= 0x2;
}

void HttpRequest::initCookies() {
  if (parser_param_flag_ & 0x4) {
    return;
  }
  std::string cookie = getHeader("cookie");
  if (cookie.empty()) {
    parser_param_flag_ |= 0x4;
    return;
  }
  PARSE_PARAM(cookie, cookies_, ';', lynx::detail::trim);
  parser_param_flag_ |= 0x4;
}

} // namespace lynx
