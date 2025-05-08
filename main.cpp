#include "netlib/src/netlib.h"
#ifdef __linux__
#include <sys/sendfile.h>
#endif
#include <print>
#include <fstream>

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

int main()
{
    netlib::server_raw server(false, 0);
    server.open_server("0.0.0.0", 8080);

    while (true)
    {
        std::vector<int> readable = server.wait_readable();
        //std::this_thread::sleep_for(std::chrono::milliseconds(50));
        for (auto user: readable)
        {
            char *data = server.receive_data_ensured(user, 3000);
            if (data)
            {
                std::string_view dat(data);
                if (dat.starts_with("GET") && dat.size() > 4)
                {
                    dat.remove_prefix(4);
                    int ins_size = 0;
                    for (auto c: dat)
                    {
                        if (c == ' ')
                            break;
                        ins_size++;
                    }
                    dat.remove_suffix(dat.size() - ins_size);
                    std::println("{}", dat);
                    if (dat == "/")
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
                    server.disconnect_user(user);
                }
                if (dat.starts_with("POST"))
                {
                    size_t pos = dat.find("Content-Length");
                    dat.remove_prefix(pos);
                    dat.remove_prefix(strlen("Content-Length: "));
                    int lenght = atoi_newline(dat.data());
                    std::println("{} {}", dat, lenght);

                    pos = dat.find("filename=");
                    if (pos == dat.npos)

                    dat.remove_prefix(pos);
                    dat.remove_prefix(strlen("filename="));
                    std::string_view file_data(data);
                    std::string filename = get_filename(dat);
                    std::println("Filename {}", filename);
                    dat.remove_prefix(dat.find("\r\n\r\n"));
                    dat.remove_prefix(dat.find("\r\n\r\n"));
                    dat.remove_prefix(4);
                    std::println("{}", dat);
                    if (lenght > 1000)
                    {
                        std::println("set a target");
                        server.set_target(user , lenght - (dat.data() - data));
                        server.wait_readable();
                        auto data_res = server.receive_everything(user);
                        char *buffer = data_res.first;
                        data = (char *)realloc(data, lenght + 1000);
                        if (!data)
                            throw std::runtime_error("Memory allocation failed");
                        memcpy(data, buffer, lenght + 1000);
                        dat = std::string_view(data);
                    }
                    std::ofstream a(filename, std::ios::binary);
                    a.write(dat.data(), lenght);
                    netlib::send_packet(std::make_tuple(std::string("HTTP/1.1 200 OK")), user);
                    server.disconnect_user(user);
                }
                free(data);
            }
            
        }
    }
}
