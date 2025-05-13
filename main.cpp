#include "netlib/src/netlib.h"
#ifdef __linux__
#include <sys/sendfile.h>
#endif
#include <print>
#include <fstream>
#include <map>

int atoi_newline(const char *data)
{
    int ret = 0;

    while (true)
    {
        ret += *data - '0';
        data++;
        if (*data == '\n' || *data == '\0' || *data == '\r')
            break;
        ret *= 10;
    }
    return ret;
}

std::string get_filename(std::string_view data)
{
    int index = 1;
    std::string ret;
    while (true)
    {
        if (data[index] == '"')
            break;
        ret.push_back(data[index]);
        index++;
    }
    return ret;
}

static std::vector<std::string> split_until_newline(const std::string &data, const char c)
{
    std::vector<std::string> ret;
    size_t index = 0;
    size_t previous_start = 0;

    while (data[index] != '\n' && data[index] != '\0')
    {
        if (data[index] == c || data[index] == '\r')
        {
            ret.push_back(data.substr(previous_start, index - previous_start));
            previous_start = index + 1;
        }
        index++;
    }
    ret.push_back(data.substr(previous_start, index - previous_start));
    return ret;
}

static std::vector<std::string> split(const std::string &data, const std::string d)
{
    std::vector<std::string> ret;
    size_t index = 0;
    size_t index2 = 0;
    size_t previous_start = 0;

    while (index < data.size())
    {
        if (data[index] == d[index2])
        {
            index2++;
            if (index2 >= d.size())
            {
                ret.push_back(data.substr(previous_start, (index - previous_start)));
                previous_start = index + 1;
                index2 = 0;
            }
        }
        else
            index2 = 0;
        index++;
    }
    ret.push_back(data.substr(previous_start, index - previous_start));
    return ret;
}

template <typename T>
void print_container(T &container)
{
    for (auto x: container)
    {
        std::println("{}", x);
    }
}

template <typename T>
void print_map(T container)
{
    for (auto [key, value]: container)
    {
        value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());
        value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());
        std::println("key {} value {}", key, value);
    }
}

std::map<std::string, std::string> parse_header(const std::string &x)
{
    std::map<std::string, std::string> ret;
    if (x.contains(": "))
    {
        std::vector<std::string> s = split(x, ": ");
        ret.insert({s[0], s[1]});
    }
    return ret;
}

std::map<std::string, std::string> parse_headers(const std::vector<std::string> &data)
{
    std::map<std::string, std::string> ret;
    for (auto &x: data)
    {
        if (x.contains(": "))
        {
            std::vector<std::string> s = split(x, ": ");
            ret.insert({s[0], s[1]});
        }
    }
    return ret;
}

int main()
{
    netlib::server_raw server(false, 0);
    server.open_server("0.0.0.0", 8080);

    while (true)
    {
        std::vector<int> readable = server.wait_readable();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        for (auto user: readable)
        {
            char * data = server.get_line(user);
            if (data)
            {
                std::string dat = std::string(data);
                std::vector<std::string> head = split(dat, " ");
                print_container(head);
                if (head[0] == "GET")
                {
                    if (head[1] == "/")
                    {
                        netlib::send_packet(std::make_tuple(std::string("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n")), user);
                        std::ifstream file("../test.html",std::ios::binary);
                        std::streampos size = file.tellg();
                        file.seekg(0, std::ios::end);
                        size = file.tellg() - size;
                        file.close();
                        int filefd = open("../test.html", O_RDONLY);
                        #ifdef __linux__
                        sendfile(user, filefd, 0, size);
                        #endif
                        #ifdef __APPLE__
                        sendfile(filefd, user, 0, (long long*)&size, nullptr, 0);
                        #endif
                    }
                }
                else if (head[0] == "POST")
                {
                    while (true)
                    {
                        char *line = server.get_line(user);
                        if (line)
                        {
                            std::string str = std::string(line);
                            auto head = parse_header(str);
                            print_map(head);
                        }
                        else
                            break;
                        free(line);
                    }
                }
                server.disconnect_user(user);
                free(data);
            }
        }
    }
}
