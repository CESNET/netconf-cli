/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#pragma once

#include <boost/mpl/vector.hpp>
#include "ast_path.hpp"
#include "ast_values.hpp"

namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;

using ascii::space;
using x3::_attr;
using x3::alnum;
using x3::alpha;
using x3::char_;
using x3::double_;
using x3::expect;
using x3::int_;
using x3::lexeme;
using x3::lit;
using x3::uint_;

struct parser_context_tag;

using keyValue_ = std::pair<std::string, std::string>;

enum class LsOption {
    Recursive
};

struct discard_ : x3::position_tagged {
    static constexpr auto name = "discard";
    static constexpr auto shortHelp = "discard - Discard current changes.";
    static constexpr auto longHelp = R"(
    discard

    Discards current changes. Accepts no arguments.

    Usage:
        /> discard)";
    bool operator==(const discard_& b) const;
};

struct ls_ : x3::position_tagged {
    static constexpr auto name = "ls";
    static constexpr auto shortHelp = "ls - List available nodes.";
    static constexpr auto longHelp = R"(
    ls [--recursive] [path]

    Lists available nodes in the current directory. Optionally
    accepts a path argument. Accepts both schema paths and data
    paths. Path starting with a forward slash means an absolute
    path.

    Usage:
        /> ls
        /> ls --recursive module:node
        /> ls /module:node)";
    bool operator==(const ls_& b) const;
    std::vector<LsOption> m_options;
    boost::optional<boost::variant<dataPath_, schemaPath_>> m_path;
};

struct cd_ : x3::position_tagged {
    static constexpr auto name = "cd";
    static constexpr auto shortHelp = "cd - Enter a different node.";
    static constexpr auto longHelp = R"(
    cd path

    Enters a node specified by path. Only accepts data paths.

    Usage:
        /> cd /module:node/node2
        /> cd ..)";
    bool operator==(const cd_& b) const;
    dataPath_ m_path;
};

struct create_ : x3::position_tagged {
    static constexpr auto name = "create";
    static constexpr auto shortHelp = "create - Create a presence container.";
    static constexpr auto longHelp = R"(
    create path_to_presence_container

    Creates a presence container specified by a path.

    Usage:
        /> create /module:pContainer)";
    bool operator==(const create_& b) const;
    dataPath_ m_path;
};

struct delete_ : x3::position_tagged {
    static constexpr auto name = "delete";
    static constexpr auto shortHelp = "delete - Delete a presence container.";
    static constexpr auto longHelp = R"(
    delete path_to_presence_container

    Delete a presence container specified by a path.

    Usage:
        /> delete /module:pContainer)";
    bool operator==(const delete_& b) const;
    dataPath_ m_path;
};

struct set_ : x3::position_tagged {
    static constexpr auto name = "set";
    static constexpr auto shortHelp = "set - Change value of a leaf.";
    static constexpr auto longHelp = R"(
    set path_to_leaf value

    Changes the leaf specified by path to value.

    Usage:
        /> set /module:leaf 123
        /> set /module:leaf abc)";
    bool operator==(const set_& b) const;
    dataPath_ m_path;
    leaf_data_ m_data;
};

struct commit_ : x3::position_tagged {
    static constexpr auto name = "commit";
    static constexpr auto shortHelp = "commit - Commit current changes.";
    static constexpr auto longHelp = R"(
    commit

    Commits the current changes. Accepts no arguments.

    Usage:
        /> commit)";
    bool operator==(const set_& b) const;
};

struct get_ : x3::position_tagged {
    static constexpr auto name = "get";
    static constexpr auto shortHelp = "get - Retrieve configuration from the server.";
    static constexpr auto longHelp = R"(
    get [path]

    Retrieves configuration of the current node. Works recursively.
    Optionally takes an argument specifying a path, the output will,
    as if the user was in that node.

    Usage:
        /> get
        /> get /module:path)";
    bool operator==(const get_& b) const;
    boost::optional<boost::variant<dataPath_, schemaPath_>> m_path;
};

struct help_;
using CommandTypes = boost::mpl::vector<discard_, ls_, cd_, create_, delete_, set_, commit_, get_, help_>;
struct help_ : x3::position_tagged {
    static constexpr auto name = "help";
    static constexpr auto shortHelp = "help - Print help for commands.";
    static constexpr auto longHelp = R"(
    help [command_name]

    Print help for command_name. If used without an argument,
    print short help for all commands.

    Usage:
        /> help
        /> help cd
        /> help help)";
    bool operator==(const help_& b) const;

    // The help command has got one optional argument – a command name (type).
    // All commands are saved in CommandTypes, so we could just use that, but
    // that way, Spirit would be default constructing the command structs,
    // which is undesirable, so firstly we use mpl::transform to wrap
    // CommandTypes with boost::type:
    using WrappedCommandTypes = boost::mpl::transform<CommandTypes, boost::type<boost::mpl::_>>::type;
    // Next, we create a variant over the wrapped types:
    using CommandTypesVariant = boost::make_variant_over<WrappedCommandTypes>::type;
    // Finally, we wrap the variant with boost::optional:
    boost::optional<CommandTypesVariant> m_cmd;
};

// TODO: The usage of MPL won't be necessary after std::variant support is added to Spirit
// https://github.com/boostorg/spirit/issues/270
using command_ = boost::make_variant_over<CommandTypes>::type;

BOOST_FUSION_ADAPT_STRUCT(ls_, m_options, m_path)
BOOST_FUSION_ADAPT_STRUCT(cd_, m_path)
BOOST_FUSION_ADAPT_STRUCT(create_, m_path)
BOOST_FUSION_ADAPT_STRUCT(delete_, m_path)
BOOST_FUSION_ADAPT_STRUCT(enum_, m_value)
BOOST_FUSION_ADAPT_STRUCT(set_, m_path, m_data)
BOOST_FUSION_ADAPT_STRUCT(commit_)
BOOST_FUSION_ADAPT_STRUCT(help_, m_cmd)
BOOST_FUSION_ADAPT_STRUCT(discard_)
BOOST_FUSION_ADAPT_STRUCT(get_, m_path)
