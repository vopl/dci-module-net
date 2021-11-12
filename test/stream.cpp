/* This file is part of the the dci project. Copyright (C) 2013-2021 vopl, shtoba.
   This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
   You should have received a copy of the GNU Affero General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. */

#include <dci/test.hpp>
#include <dci/host.hpp>
#include <dci/poll.hpp>
#include <dci/utils/s2f.hpp>
#include "net.hpp"

using namespace dci;
using namespace dci::host;
using namespace dci::cmt;
using namespace dci::idl::net;

namespace
{
    void sleep(int ms)
    {
        dci::poll::WaitableTimer t{std::chrono::milliseconds{ms}};
        t.start();
        t.wait();
    }
}


/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_net, stream)
{
    Manager* manager = testManager();
    Host<> netHost = manager->createService<Host<>>().value();

    stream::Server<> srv;
    stream::Client<> cln;
    Endpoint srvEndpoint;

    //server listen
    {
        srv = netHost->streamServer().value();

        //bad address
        EXPECT_THROW((srv->listen(Ip4Endpoint{{192,0,2,1}, 2048})).value(), AddressNotAvailable);

        //good address
        EXPECT_NO_THROW((srv->listen(Ip4Endpoint{{127,0,0,1}, 0}).value()));
        EXPECT_NO_THROW((srv->localEndpoint().value()));
        EXPECT_EQ((srv->localEndpoint().value()).get<Ip4Endpoint>().address.octets, (Array<uint8, 4>{127,0,0,1}));
        EXPECT_NE((srv->localEndpoint().value()).get<Ip4Endpoint>().port, 0u);
        srv->close();
        dci::utils::S2f{srv->closed()}.wait();

        //data dropped after close
        EXPECT_NO_THROW((srv->localEndpoint().value()));
        EXPECT_TRUE(std::holds_alternative<NullEndpoint>(srv->localEndpoint().value()));

        //run for later usage
        EXPECT_NO_THROW((srv->listen(Ip4Endpoint{{127,0,0,1}, 0})));
        srvEndpoint = srv->localEndpoint().value();
    }

    //client
    {
        cln = netHost->streamClient().value();

        //bad address
        EXPECT_NO_THROW((cln->bind(Ip4Endpoint{{0,0,0,0}, 0})).value());
        EXPECT_THROW((cln->connect(Ip4Endpoint{{255,255,255,255}, 0})).value(), NetworkUnreachable);

        //bad address
        EXPECT_NO_THROW((cln->bind(srvEndpoint)).value());
        EXPECT_THROW((cln->connect(srvEndpoint)).value(), AddressInIse);

        //good address
        EXPECT_NO_THROW((cln->bind(Ip4Endpoint{{0,0,0,0}, 0})).value());
        EXPECT_NO_THROW((cln->connect(srvEndpoint).value()));
    }

    //channel emits 'closed' and 'failed' signals
    {
        sbs::Owner owner;

        int clos1 = 0;
        int clos2 = 0;
        int fail1 = 0;
        int fail2 = 0;

        stream::Channel<> ch1;
        srv->accepted() += owner * [&](stream::Channel<> ch)
        {
            ch1 = ch;
            ch1->closed() += owner * [&]()
            {
                clos1++;
            };
            ch1->failed() += owner * [&](auto)
            {
                fail1++;
            };
        };

        stream::Channel<> ch2 = cln->connect(srvEndpoint).value();
        ch2->closed() += owner * [&]()
        {
            clos2++;
        };
        ch2->failed() += owner * [&](auto)
        {
            fail2++;
        };

        EXPECT_EQ(clos1, 0);
        EXPECT_EQ(clos2, 0);
        EXPECT_EQ(fail1, 0);
        EXPECT_EQ(fail2, 0);

        ch1->close();
        sleep(1);
        EXPECT_EQ(clos1, 1);
        EXPECT_EQ(clos2, 1);
        EXPECT_EQ(fail1, 0);
        EXPECT_EQ(fail2, 0);

        ch2->close();
        sleep(1);
        EXPECT_EQ(clos1, 1);
        EXPECT_EQ(clos2, 1);
        EXPECT_EQ(fail1, 0);
        EXPECT_EQ(fail2, 0);
    }

    //channel read/write
    {
        sbs::Owner owner;

        int clos1 = 0;
        int clos2 = 0;
        int fail1 = 0;
        int fail2 = 0;
        int fin1 = 0;
        int fin2 = 0;

        stream::Channel<> ch1;
        srv->accepted() += owner * [&](stream::Channel<> ch)
        {
            ch1 = ch;
            ch1->closed() += owner * [&]()
            {
                clos1++;
            };
            ch1->failed() += owner * [&](auto)
            {
                fail1++;
            };
            ch1->received() += owner * [&](Bytes data)
            {
                EXPECT_EQ(data.size(), 1u);
                char c;
                data.begin().read(&c, 1);

                EXPECT_TRUE(c >= '0' && c <= '9');

                if(c <= '8')
                {
                    char c2 = c+1;
                    data.begin().write(&c2, 1);
                }
                if(c <= '9')
                {
                    ch1->send(std::move(data));
                }

                if(c >= '9')
                {
                    fin1++;
                    ch1->close();
                }
            };
            ch1->startReceive();
        };

        stream::Channel<> ch2 = cln->connect(srvEndpoint).value();
        ch2->closed() += owner * [&]()
        {
            clos2++;
        };
        ch2->failed() += owner * [&](auto)
        {
            fail2++;
        };
        ch2->received() += owner * [&](Bytes data)
        {
            EXPECT_EQ(data.size(), 1u);
            char c;
            data.begin().read(&c, 1);

            EXPECT_TRUE(c >= '0' && c <= '9');

            if(c <= '8')
            {
                char c2 = c+1;
                data.begin().write(&c2, 1);
            }
            if(c <= '9')
            {
                ch2->send(std::move(data));
            }

            if(c >= '9')
            {
                fin2++;
                ch2->close();
            }
        };
        ch2->startReceive();

        ch2->send(Bytes("0"));

        while(!clos1)
        {
            sleep(1);
        }
        EXPECT_EQ(clos1, 1);
        EXPECT_EQ(clos2, 1);
        EXPECT_EQ(fail1, 0);
        EXPECT_EQ(fail2, 0);
        EXPECT_EQ(fin1, 1);
        EXPECT_EQ(fin2, 1);
    }

    //server reports 'closed'
    {
        sbs::Owner owner;

        int cnt = 0;
        srv->closed() += owner*[&]
        {
            cnt++;
        };

        srv->close();
        dci::cmt::yield();
        EXPECT_EQ(cnt, 1);

        srv->close();
        dci::cmt::yield();
        EXPECT_EQ(cnt, 1);
    }
}










/*
/////////0/////////1/////////2/////////3/////////4/////////5/////////6/////////7
TEST(module_net, server)
{
    Manager* manager = testManager();
    dbgAssert(manager);

    Host<> netHost = manager->createService<Host<>>().value();

    Endpoint bindEndpoint = Ip4Endpoint {{127,0,0,1}, 0};
    Ip4Endpoint srvEndpoint;
    Event serverReady;

    stream::Server<> srv;
    stream::Client<> cl;

    cmt::spawn() += [&]
    {
        srv = netHost->streamServer().value();
        srv->listen(bindEndpoint).value();

        srvEndpoint = srv->localEndpoint().value().get<Ip4Endpoint>();
        std::cout<<"srv at: "<<srvEndpoint.port<<std::endl;
        serverReady.raise();

        srv->accepted() += [](stream::Channel<> ch)
        {
            ch->send("hello\n");
            ch->received() += [=](Bytes data) mutable
            {
                std::cout<<"srv: "<<data.toString();
                ch->send(std::move(data));
            });
        });
    };

    cmt::spawn() += [&]
    {
        cl = netHost->streamClient().value();

        serverReady.wait();

        //cl.bind(LocalEndpoint {"/tmp/221"});

        stream::Channel<> ch = cl->connect(srvEndpoint).value();
        ch->received() += [=](Bytes data) mutable
        {
            std::cout<<"cln: "<<data.toString();
            ch->send(std::move(data));
        });
    };

    dci::poll::WaitableTimer t{std::chrono::seconds{10}};
    t.start();
    t.wait();
}
//*/
