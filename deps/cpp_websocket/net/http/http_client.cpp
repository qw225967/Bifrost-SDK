#include "http_client.hpp"
#include "utils/stringex.hpp"

#include <string>
#include <sstream>
#include <uv.h>
#include <assert.h>

namespace cpp_streamer
{

HttpClient::HttpClient(uv_loop_t* loop,
                       const std::string& host,
                       uint16_t port,
                       HttpClientCallbackI* cb,
                       bool ssl_enable): host_(host)
                                         , port_(port)
                                         , cb_(cb)
{
    std::cout<<"http client 111"<<std::endl;
    client_ = new TcpClient(loop, this, ssl_enable);
    std::cout<<"http client 222"<<std::endl;
}

HttpClient::~HttpClient()
{
    if (client_) {
        delete client_;
        client_ = nullptr;
    }
}

int HttpClient::Get(const std::string& subpath, const std::map<std::string, std::string>& headers) {
    method_  = HTTP_GET;
    subpath_ = subpath;
    headers_ = headers;

    client_->Connect(host_, port_);
    return 0;
}

int HttpClient::Post(const std::string& subpath, const std::map<std::string, std::string>& headers, const std::string& data) {
    method_    = HTTP_POST;
    subpath_   = subpath;
    post_data_ = data;
    headers_   = headers;

    client_->Connect(host_, port_);
    return 0;
}

void HttpClient::Close() {
    client_->Close();
}

TcpClient* HttpClient::GetTcpClient() {
    return client_;
}

void HttpClient::OnConnect(int ret_code) {
    if (ret_code < 0) {
        std::shared_ptr<HttpClientResponse> resp_ptr;
        cb_->OnHttpRead(ret_code, resp_ptr);
        return;
    }
    std::stringstream http_stream;

    if (method_ == HTTP_GET) {
        http_stream << "GET " << subpath_ << " HTTP/1.1\r\n";
    } else if (method_ == HTTP_POST) {
        http_stream << "POST " << subpath_ << " HTTP/1.1\r\n";
    } else {
    }
    http_stream << "Accept: */*\r\n";
    http_stream << "Host: " << host_ << "\r\n";
    for (auto& header : headers_) {
        http_stream << header.first << ": " << header.second << "\r\n";
    }
    if (method_ == HTTP_POST) {
        http_stream << "Content-Length: " << post_data_.length() << "\r\n";
    }
    http_stream << "\r\n";
    if (method_ == HTTP_POST) {
        http_stream << post_data_;
    }
    client_->Send(http_stream.str().c_str(), http_stream.str().length());
}

void HttpClient::OnWrite(int ret_code, size_t sent_size) {
    if (ret_code == 0) {
        client_->AsyncRead();
    }
}

void HttpClient::OnRead(int ret_code, const char* data, size_t data_size) {
    if (ret_code < 0) {
        cb_->OnHttpRead(ret_code, resp_ptr_);
        return;
    }

    if (data_size == 0) {
        cb_->OnHttpRead(-2, resp_ptr_);
        return;
    }

    if (!resp_ptr_) {
        resp_ptr_ = std::make_shared<HttpClientResponse>();
    }

    resp_ptr_->data_.AppendData(data, data_size);

		bool has_websocket_together = false;
		int tcp_coalescence_offset = 0;
    if (!resp_ptr_->header_ready_) {
        std::string header_str(resp_ptr_->data_.Data(), resp_ptr_->data_.DataLen());
        size_t pos = header_str.find("\r\n\r\n");
				// 表明有数据粘包发送
				has_websocket_together = data_size > pos + 4;
				tcp_coalescence_offset = has_websocket_together ? (data_size - 4 - pos) : 0;

        if (pos != std::string::npos) {
            std::vector<std::string> lines_vec;
            resp_ptr_->header_ready_ = true;
            header_str = header_str.substr(0, pos);
            resp_ptr_->data_.ConsumeData(pos + 4);

            StringSplit(header_str, "\r\n", lines_vec);
            for (size_t i = 0; i < lines_vec.size(); i++) {
                if (i == 0) {
                    std::vector<std::string> item_vec;
                    StringSplit(lines_vec[0], " ", item_vec);
                    assert(item_vec.size() >= 3);

                    pos = item_vec[0].find("/");
                    assert(pos != std::string::npos);
                    resp_ptr_->proto_   = item_vec[0].substr(0, pos);
                    resp_ptr_->version_ = item_vec[0].substr(pos+1);
                    resp_ptr_->status_code_ = atoi(item_vec[1].c_str());
                    if (item_vec.size() == 3) {
                        resp_ptr_->status_      = item_vec[2];
                    } else {
                        std::string status_string("");
                        for (size_t i = 2; i < item_vec.size(); i++) {
                            status_string += item_vec[i];
                        }
                        resp_ptr_->status_ = status_string;
                    }
                    continue;
                }
                pos = lines_vec[i].find(":");
                assert(pos != std::string::npos);
                std::string key   = lines_vec[i].substr(0, pos);
                std::string value = lines_vec[i].substr(pos + 2);

                if (key == "Content-Length") {
                    resp_ptr_->content_length_ = atoi(value.c_str());
                }
                resp_ptr_->headers_[key] = value;
            }
        } else {
            client_->AsyncRead();
            return;
        }
    }

    if (resp_ptr_->content_length_ > 0) {
        if ((int)resp_ptr_->data_.DataLen() >= resp_ptr_->content_length_) {
            resp_ptr_->body_ready_ = true;
            cb_->OnHttpRead(0, resp_ptr_);
        } else {
            client_->AsyncRead();
        }
    } else {
        cb_->OnHttpRead(0, resp_ptr_);
        client_->AsyncRead();
    }

		if (has_websocket_together) {
				this->OnRead(0, data + (data_size - tcp_coalescence_offset), tcp_coalescence_offset);
		}
}

}
