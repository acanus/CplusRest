#pragma warning(disable : 4996)
#define _TURN_OFF_PLATFORM_STRING
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include  <boost/signals2.hpp>;
#include <cpprest/filestream.h>
#include <opencv2/opencv.hpp> 
#include <iostream> 
#include <opencv2/imgproc.hpp>
#include <chrono> 
#include <list> 
#include <iterator> 
#include <ppl.h>
#include <thread>
#include <cpprest/producerconsumerstream.h>
using namespace std::chrono;
using namespace concurrency;
using namespace cv;
using namespace std;

using namespace concurrency;
using namespace concurrency::streams;

using namespace web;
using namespace web::json;
using namespace web::http;
using namespace web::http::experimental::listener;
#include <iostream>
#include <map>
#include <set>
#include <string>
using namespace std;


#define TRACE(msg)            wcout << msg
#define TRACE_ACTION(a, k, v) wcout << a << L" (" << k << L", " << v << L")\n"

map<utility::string_t, utility::string_t> dictionary;
struct HelloWorld
{
    void operator()() const
    {
        std::cout << "Hello, World!" << std::endl;
    }
};


void display_json(
    json::value const& jvalue,
    utility::string_t const& prefix)
{
    wcout << prefix << jvalue.serialize() << endl;
}

void handle_get(http_request request)
{
    TRACE(L"\nhandle GET\n");

    auto answer = json::value::object();

    for (auto const& p : dictionary)
    {
        answer[p.first] = json::value::string(p.second);
    }

    display_json(json::value::null(), L"R: ");
    display_json(answer, L"S: ");

    request.reply(status_codes::OK, answer);
}
vector<uint8_t> Encode( std::string mystring) {
    std::vector<uint8_t> buffer(mystring.length());
    memcpy(&buffer[0], mystring.data(), mystring.length());
    return buffer;
}
//event stream
vector<uint8_t> CreateHeader(int length)
{ 
    std::string header = "--frame\r\nContent-Type:image/jpeg\r\nContent-Length:"+ to_string(length)+"\r\n\r\n";
    
    return Encode(header);
}
vector<uint8_t> CreateFooter()
{
    return Encode("\r\n");
}
void WriteFrame(producer_consumer_buffer<uint8_t> stream, vector<unsigned char> data)
{

    auto header = CreateHeader(data.size());
    auto footer = CreateFooter();
    stream.putn_nocopy(header.data(), (header.size()));
    stream.putn_nocopy(data.data(), data.size());
    stream.putn_nocopy(footer.data(),footer.size());
    stream.sync().wait();
}
vector<uint8_t> RotateImage(Mat image,int code) {
    Mat imagerotate;
    cv::rotate(image, imagerotate, code);
    std::vector<unsigned char> bytes;
    imencode(".jpg", imagerotate, bytes); 
        return bytes;
}
void handle_get_stream(http_request request)
{
    TRACE(L"\nhandle GET\n");
    streams::producer_consumer_buffer<uint8_t> rwbuf;
    streams::basic_istream<uint8_t> stream(rwbuf);  
    http_response response(status_codes::OK);
    response.set_body(stream);
    response.headers().add(header_names::access_control_allow_origin, _XPLATSTR("*"));
    response.headers().add(header_names::content_type, _XPLATSTR("multipart/x-mixed-replace; boundary=frame"));
    response.headers().add(header_names::connection, _XPLATSTR("keep-alive"));
    auto rep = request.reply(response);
    std::thread t1([](streams::producer_consumer_buffer<uint8_t>  rwbuf) {
        
            
           
            auto vid=cv::VideoCapture(0);
            while (true)
            {
                if (vid.isOpened())
                {
                    Mat imread;
                    if (vid.read(imread))
                    {
                        std::vector<unsigned char> bytes;
                        imencode(".jpg", imread, bytes);
                        WriteFrame(rwbuf, bytes);
                        rwbuf.sync().wait();
                    }
                    else {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            
            
            
            //rwbuf.close();
        
    }, rwbuf);
   
    
    t1.join();
    rep.wait();
    
}
void handle_get_image_stream(http_request request)
{
    TRACE(L"\nhandle GET\n");
    streams::producer_consumer_buffer<char> rwbuf;
    streams::basic_istream<uint8_t> stream(rwbuf);
    http_response response(status_codes::OK);
    response.set_body(stream);
    response.headers().add(header_names::access_control_allow_origin, _XPLATSTR("*"));
    response.headers().add(header_names::content_type, _XPLATSTR("text/event-stream"));
    auto rep = request.reply(response);
    std::thread t1([](streams::producer_consumer_buffer<char>  rwbuf) {


        rwbuf.putn_nocopy("data: aaa \n\n", 12).wait();
        rwbuf.sync().wait();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        rwbuf.putn_nocopy("data: aaa \n\n", 12).wait();
        rwbuf.sync().wait();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        rwbuf.putn_nocopy("data: aaa \n\n", 12).wait();
        rwbuf.sync().wait();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        //rwbuf.close();

    }, rwbuf);


    t1.join();
    rep.wait();

}

void handle_get1(http_request request)
{
    TRACE(L"\nhandle GET 1\n");
    auto http_get_vars = uri::split_query(request.request_uri().query());
    auto answer = json::value::object();
   
    auto found_name = http_get_vars.find(_XPLATSTR("value"));
    auto aa =found_name->second;
    int myNr = std::stoi(aa);
    for (auto const& p : dictionary)
    {
        answer[p.first] = json::value::string(p.second);
    }

    display_json(json::value::null(), L"R: ");
    display_json(answer, L"S:1 ");

    request.reply(status_codes::OK, answer);
}
void handle_request(
    http_request request,
    function<void(json::value const&, json::value&)> action)
{
    auto answer = json::value::object();

    request
        .extract_json()
        .then([&answer, &action](pplx::task<json::value> task) {
        try
        {
            auto const& jvalue = task.get();
            display_json(jvalue, L"R: ");

            if (!jvalue.is_null())
            {
                action(jvalue, answer);
            }
        }
        catch (http_exception const& e)
        {
            wcout << e.what() << endl;
        }
    })
        .wait();


    display_json(answer, L"S: ");

    request.reply(status_codes::OK, answer);
}

void handle_post(http_request request)
{
    TRACE("\nhandle POST\n");

    handle_request(
        request,
        [](json::value const& jvalue, json::value& answer)
    {
        for (auto const& e : jvalue.as_array())
        {
            if (e.is_string())
            {
                auto key = e.as_string();
                auto pos = dictionary.find(key);

                if (pos == dictionary.end())
                {
                    answer[key] = json::value::string(L"<nil>");
                }
                else
                {
                    answer[pos->first] = json::value::string(pos->second);
                }
            }
        }
    });
}

void handle_put(http_request request)
{
    TRACE("\nhandle PUT\n");

    handle_request(
        request,
        [](json::value const& jvalue, json::value& answer)
    {
        for (auto const& e : jvalue.as_object())
        {
            if (e.second.is_string())
            {
                auto key = e.first;
                auto value = e.second.as_string();

                if (dictionary.find(key) == dictionary.end())
                {
                    TRACE_ACTION(L"added", key, value);
                    answer[key] = json::value::string(L"<put>");
                }
                else
                {
                    TRACE_ACTION(L"updated", key, value);
                    answer[key] = json::value::string(L"<updated>");
                }

                dictionary[key] = value;
            }
        }
    });
}

void handle_del(http_request request)
{
    TRACE("\nhandle DEL\n");

    handle_request(
        request,
        [](json::value const& jvalue, json::value& answer)
    {
        set<utility::string_t> keys;
        for (auto const& e : jvalue.as_array())
        {
            if (e.is_string())
            {
                auto key = e.as_string();

                auto pos = dictionary.find(key);
                if (pos == dictionary.end())
                {
                    answer[key] = json::value::string(L"<failed>");
                }
                else
                {
                    TRACE_ACTION(L"deleted", pos->first, pos->second);
                    answer[key] = json::value::string(L"<deleted>");
                    keys.insert(key);
                }
            }
        }

        for (auto const& key : keys)
            dictionary.erase(key);
    });
}

int main()
{
    http_listener listener(L"http://192.168.1.71/restdemo");


    listener.support(methods::GET, handle_get_stream);
    listener.support(methods::POST, handle_post);
    listener.support(methods::PUT, handle_put);
    listener.support(methods::DEL, handle_del);
    // ...

// Signal with no arguments and a void return value
    boost::signals2::signal<void()> sig;

    // Connect a HelloWorld slot
    HelloWorld hello;
    sig.connect(hello);

    // Call all of the slots
    sig(); 
    try
    {
        listener
            .open()
            .then([&listener]() {TRACE(L"\nstarting to listen\n"); })
            .wait();

        while (true);
    }
    catch (std::exception  const& e)
    {
        wcout << e.what() << endl;
    }

    return 0;
}