/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include <docopt.h>
#include <experimental/filesystem>
#include <iostream>
#include "NETCONF_CLI_VERSION.h"
#include "interpreter.hpp"
#include "yang_schema.hpp"

const char* schema = R"(
module example-schema {
    namespace "http://example.com/example-sports";
    prefix coze;

    container a {
        container a2 {
            container a3 {
                presence true;
            }
        }

        leaf leafa {
            type string;
        }
    }

    container b {
        container b2 {
            presence true;
            container b3 {
            }
        }
    }

    leaf leafString {
        type string;
    }

    leaf leafDecimal {
        type decimal64 {
            fraction-digits 5;
        }
    }

    leaf leafBool {
        type boolean;
    }

    leaf leafInt {
        type int32;
    }

    leaf leafUint {
        type uint32;
    }

    leaf leafEnum {
        type enumeration {
            enum lol;
            enum data;
            enum coze;
        }
    }

    list _list {
        key number;

        leaf number {
            type int32;
        }

        container contInList {
            presence true;
        }
    }

    list twoKeyList {
        key "name number";

        leaf number {
            type int32;
        }

        leaf name {
            type string;
        }
    }
})";

static const char usage[] =
    R"(CLI interface to remote NETCONF hosts

Usage:
  netconf-cli <path-to-yang-schema>...
  netconf-cli (-h | --help)
  netconf-cli --version
)";


int main(int argc, char* argv[])
{
    auto args = docopt::docopt(usage,
                               {argv + 1, argv + argc},
                               true,
                               "netconf-cli " NETCONF_CLI_VERSION,
                               true);
    std::cout << "Welcome to netconf-cli" << std::endl;

    auto yangschema = std::make_shared<YangSchema>();
    yangschema->addSchemaString(schema);
    Parser parser(yangschema);

    //SysrepoAccess datastore("app1");

    //for (auto it : args.at("<path-to-yang-schema>").asStringList()) {
    //    auto dir = std::experimental::filesystem::absolute(it).remove_filename();
    //    yangschema->addSchemaDirectory(dir.c_str());
    //    yangschema->addSchemaFile(it.c_str());
    //}

    while (true) {
        std::cout << parser.currentNode() << "> ";
        std::string input;
        std::getline(std::cin, input);
        if (std::cin.eof())
            break;

        try {
            command_ cmd = parser.parseCommand(input, std::cout);
            //boost::apply_visitor(Interpreter(parser, datastore), cmd);
        } catch (InvalidCommandException& ex) {
            std::cerr << ex.what() << std::endl;
        }
    }

    return 0;
}
