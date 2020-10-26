/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#pragma once

#include <boost/mpl/vector.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include "ast_path.hpp"
#include "ast_values.hpp"
#include "yang_operations.hpp"

namespace x3 = boost::spirit::x3;

using keyValue_ = std::pair<std::string, leaf_data_>;

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
    boost::optional<boost::variant<dataPath_, schemaPath_, module_>> m_path;
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
    create path

    Creates a presence container or a list instance specified by path.

    Usage:
        /> create /module:pContainer
        /> create /module:list[key=value][anotherKey=value])";
    bool operator==(const create_& b) const;
    dataPath_ m_path;
};

struct delete_ : x3::position_tagged {
    static constexpr auto name = "delete";
    static constexpr auto shortHelp = "delete - Delete a presence container.";
    static constexpr auto longHelp = R"(
    delete path

    Deletes a presence container or a list instance specified by path.

    Usage:
        /> delete /module:pContainer
        /> delete /module:list[key=value][anotherKey=value])";
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
    boost::optional<boost::variant<dataPath_, module_>> m_path;
};

struct describe_ : x3::position_tagged {
    static constexpr auto name = "describe";
    static constexpr auto shortHelp = "describe - Print information about YANG tree path.";
    static constexpr auto longHelp = R"(
    describe <path>

    Show documentation of YANG tree paths. In the YANG model, each item may
    have an optional `description` which often explains the function of that
    node to the end user. This command takes the description from the YANG
    model and shows it to the user along with additional data, such as the type
    of the node, units of leaf values, etc.

    Usage:
        /> describe /module:node)";
    bool operator==(const describe_& b) const;

    boost::variant<schemaPath_, dataPath_> m_path;
};

struct copy_ : x3::position_tagged {
    static constexpr auto name = "copy";
    static constexpr auto shortHelp = "copy - copy configuration datastores around";
    static constexpr auto longHelp = R"(
    copy <source> <destination>

    Usage:
        /> copy running startup
        /> copy startup running)";
    bool operator==(const copy_& b) const;

    Datastore m_source;
    Datastore m_destination;
};

enum class MoveMode {
    Begin,
    End,
    Before,
    After
};

struct move_ : x3::position_tagged {
    static constexpr auto name = "move";
    static constexpr auto shortHelp = "move - move (leaf)list instances around";
    static constexpr auto longHelp = R"(
    move <list-instance-path> begin
    move <list-instance-path> end
    move <list-instance-path> before <key>
    move <list-instance-path> after <key>

    Usage:
        /> move mod:leaflist['abc'] begin
        /> move mod:leaflist['def'] after 'abc'
        /> move mod:interfaces['eth0'] after ['eth1'])";
    bool operator==(const move_& b) const;

    dataPath_ m_source;

    std::variant<yang::move::Absolute, yang::move::Relative> m_destination;
};

struct dump_ : x3::position_tagged {
    static constexpr auto name = "dump";
    static constexpr auto shortHelp = "dump - dump entire content of the datastore";
    static constexpr auto longHelp = R"(
    dump xml|json

    Usage:
        /> dump xml
        /> dump json)";
    bool operator==(const dump_& other) const;

    DataFormat m_format;
};

struct prepare_ : x3::position_tagged {
    static constexpr auto name = "prepare";
    static constexpr auto shortHelp = "prepare - initiate RPC or action";
    static constexpr auto longHelp = R"(
    prepare <path-to-rpc-or-action>

    This command puts you into a mode where you can set your input parameters.

    Usage:
        /> prepare <path-to-rpc-or-action>)";
    bool operator==(const prepare_& other) const;

    dataPath_ m_path;
};

struct exec_ : x3::position_tagged {
    static constexpr auto name = "exec";
    static constexpr auto shortHelp = "exec - execute RPC";
    static constexpr auto longHelp = R"(
    exec

    This command executes the RPC you have previously initiated.

    Usage:
        /> exec)";
    bool operator==(const exec_& other) const;
};

struct cancel_ : x3::position_tagged {
    static constexpr auto name = "cancel";
    static constexpr auto shortHelp = "cancel - cancel an ongoing RPC input";
    static constexpr auto longHelp = R"(
    cancel

    This command cancels a previously entered RPC context.

    Usage:
        /> cancel)";
    bool operator==(const cancel_& other) const;
};

struct help_;
using CommandTypes = boost::mpl::vector<cancel_, cd_, commit_, copy_, create_, delete_, describe_, discard_, dump_, exec_, get_, help_, ls_, move_, prepare_, set_>;
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
BOOST_FUSION_ADAPT_STRUCT(set_, m_path, m_data)
BOOST_FUSION_ADAPT_STRUCT(enum_, m_value)
BOOST_FUSION_ADAPT_STRUCT(bits_, m_bits)
BOOST_FUSION_ADAPT_STRUCT(binary_, m_value)
BOOST_FUSION_ADAPT_STRUCT(identityRef_, m_prefix, m_value)
BOOST_FUSION_ADAPT_STRUCT(commit_)
BOOST_FUSION_ADAPT_STRUCT(describe_, m_path)
BOOST_FUSION_ADAPT_STRUCT(help_, m_cmd)
BOOST_FUSION_ADAPT_STRUCT(discard_)
BOOST_FUSION_ADAPT_STRUCT(get_, m_path)
BOOST_FUSION_ADAPT_STRUCT(copy_, m_source, m_destination)
BOOST_FUSION_ADAPT_STRUCT(move_, m_source, m_destination)
BOOST_FUSION_ADAPT_STRUCT(dump_, m_format)
BOOST_FUSION_ADAPT_STRUCT(prepare_, m_path)
