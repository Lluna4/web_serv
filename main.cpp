#include "netlib/src/netlib.h"
#include <sys/sendfile.h>
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

int main()
{
    netlib::server_raw server;
    server.open_server("0.0.0.0", 8080);

    while (true)
    {
        std::vector<int> readable = server.get_readable();
        for (auto user: readable)
        {
            char *data = server.receive_everything(user);
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
                    if (dat.compare("/") == 0)
                    {
                        netlib::send_packet(std::make_tuple(std::string("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n")), user);
                        std::ifstream file("../test.html",std::ios::binary);
                        std::streampos size = file.tellg();
                        file.seekg(0, std::ios::end);
                        size = file.tellg() - size;
                        file.close();
                        int filefd = open("../test.html", O_RDONLY);
                        sendfile(user, filefd, 0, size);
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
                    pos = dat.find("image/png");
                    dat.remove_prefix(pos);
                    dat.remove_prefix(strlen("image/png\r\n") + 2);
                    std::ofstream a("Test.png", std::ios::binary);
                    a.write(dat.data(), lenght);
                    netlib::send_packet(std::make_tuple(std::string("HTTP/1.1 201 CREATED")), user);
                    server.disconnect_user(user);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
