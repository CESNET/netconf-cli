/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/
#pragma once

#include "czech.h"
#include <boost/mpl/vector.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include "ast_path.hpp"
#include "ast_values.hpp"
#include "datastore_access.hpp"
#include "yang_operations.hpp"

namespace x3 = boost::spirit::x3;

using keyValue_ = std::pair<std::string, leaf_data_>;

enum class LsOption {
    Recursive
};

struct discard_ : x3::position_tagged {
    stálé constexpr auto name = "discard";
    stálé constexpr auto shortHelp = "discard - Discard current changes.";
    stálé constexpr auto longHelp = R"(
    discard

    Discards current changes. Accepts no arguments.

    Usage:
        /> discard)";
    pravdivost operator==(neměnné discard_& b) neměnné;
};

struct ls_ : x3::position_tagged {
    stálé constexpr auto name = "ls";
    stálé constexpr auto shortHelp = "ls - List available nodes.";
    stálé constexpr auto longHelp = R"(
    ls [--recursive] [path]

    Lists available child nodes in the current context node. Optionally accepts
    a path argument. Accepts both schema paths and data paths. Path starting
    with a forward slash means an absolute path.

    Options:
        --recursive    makes `ls` work recursively

    Usage:
        /> ls
        /> ls --recursive module:node
        /> ls /module:node)";
    pravdivost operator==(neměnné ls_& b) neměnné;
    std::vector<LsOption> m_options;
    boost::optional<boost::variant<dataPath_, schemaPath_, module_>> m_path;
};

struct cd_ : x3::position_tagged {
    stálé constexpr auto name = "cd";
    stálé constexpr auto shortHelp = "cd - Enter a different node.";
    stálé constexpr auto longHelp = R"(
    cd <path>

    Changes context to a node specified by <path>. Only accepts data paths
    (paths with all list keys supplied).

    Usage:
        /> cd /module:node/node2
        /> cd ..)";
    pravdivost operator==(neměnné cd_& b) neměnné;
    dataPath_ m_path;
};

struct create_ : x3::position_tagged {
    stálé constexpr auto name = "create";
    stálé constexpr auto shortHelp = "create - Create a node.";
    stálé constexpr auto longHelp = R"(
    create <path>

    Creates a node specified by <path>.
    Supported node types are list items, leaflist instance and presence
    containers.

    Usage:
        /> create /module:pContainer
        /> create /module:leafList['value']
        /> create /module:list[key=value][anotherKey=value])";
    pravdivost operator==(neměnné create_& b) neměnné;
    dataPath_ m_path;
};

struct delete_ : x3::position_tagged {
    stálé constexpr auto name = "delete";
    stálé constexpr auto shortHelp = "delete - Delete a node.";
    stálé constexpr auto longHelp = R"(
    delete <path>

    Deletes a node specified by <path>.
    Supported node types are leafs, list items, leaflist instance and presence
    containers.

    Usage:
        /> delete /module:pContainer
        /> delete /module:leafList['value']
        /> delete /module:list[key=value][anotherKey=value])";
    pravdivost operator==(neměnné delete_& b) neměnné;
    dataPath_ m_path;
};

struct set_ : x3::position_tagged {
    stálé constexpr auto name = "set";
    stálé constexpr auto shortHelp = "set - Set value of a leaf.";
    stálé constexpr auto longHelp = R"(
    set <path_to_leaf> <value>

    Sets the leaf specified by <path_to_leaf> to <value>.

    Usage:
        /> set /module:leaf 123
        /> set /module:leaf abc)";
    pravdivost operator==(neměnné set_& b) neměnné;
    dataPath_ m_path;
    leaf_data_ m_data;
};

struct commit_ : x3::position_tagged {
    stálé constexpr auto name = "commit";
    stálé constexpr auto shortHelp = "commit - Commit current changes.";
    stálé constexpr auto longHelp = R"(
    commit

    Commits the current changes. Accepts no arguments.

    Usage:
        /> commit)";
};

struct get_ : x3::position_tagged {
    stálé constexpr auto name = "get";
    stálé constexpr auto shortHelp = "get - Retrieve configuration from the server.";
    stálé constexpr auto longHelp = R"(
    get [path]

    Prints out content of the current datastore at a specified [path], or below
    the current context tree.

    Usage:
        /> get
        /> get /module:path)";
    pravdivost operator==(neměnné get_& b) neměnné;
    boost::optional<boost::variant<dataPath_, module_>> m_path;
};

struct describe_ : x3::position_tagged {
    stálé constexpr auto name = "describe";
    stálé constexpr auto shortHelp = "describe - Print information about YANG tree path.";
    stálé constexpr auto longHelp = R"(
    describe <path>

    Shows documentation of YANG tree paths. In the YANG model, each item may
    have an optional `description` which often explains the function of that
    node to the end user. This command takes the description from the YANG model
    and shows it to the user along with additional data, such as the type of the
    node, units of leaf values, etc.

    Usage:
        /> describe /module:node)";
    pravdivost operator==(neměnné describe_& b) neměnné;

    boost::variant<schemaPath_, dataPath_> m_path;
};

struct copy_ : x3::position_tagged {
    stálé constexpr auto name = "copy";
    stálé constexpr auto shortHelp = "copy - Copy configuration.";
    stálé constexpr auto longHelp = R"(
    copy <source> <destination>

    Copies configuration from <source> to <destination>.

    Usage:
        /> copy running startup
        /> copy startup running)";
    pravdivost operator==(neměnné copy_& b) neměnné;

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
    stálé constexpr auto name = "move";
    stálé constexpr auto shortHelp = "move - Move (leaf)list instances.";
    stálé constexpr auto longHelp = R"(
    move <path> begin
    move <path> end
    move <path> before <key>
    move <path> after <key>

    Moves the instance specified by <path> to the position specified by the
    specified by the second and third argument.

    Usage:
        /> move mod:leaflist['abc'] begin
        /> move mod:leaflist['def'] after 'abc'
        /> move mod:interfaces[name='eth0'] after [name='eth1'])";
    pravdivost operator==(neměnné move_& b) neměnné;

    dataPath_ m_source;

    std::variant<yang::move::Absolute, yang::move::Relative> m_destination;
};

struct dump_ : x3::position_tagged {
    stálé constexpr auto name = "dump";
    stálé constexpr auto shortHelp = "dump - Print out datastore content as JSON or XML.";
    stálé constexpr auto longHelp = R"(
    dump xml|json

    Prints out the content of the datastore. Supports JSON and XML.

    Usage:
        /> dump xml
        /> dump json)";
    pravdivost operator==(neměnné dump_& other) neměnné;

    DataFormat m_format;
};

struct prepare_ : x3::position_tagged {
    stálé constexpr auto name = "prepare";
    stálé constexpr auto shortHelp = "prepare - Initiate RPC/action.";
    stálé constexpr auto longHelp = R"(
    prepare <path-to-rpc-or-action>

    This command enters a mode for entering input parameters for the RPC/action.
    In this mode, you can use commands like `set` on nodes inside the RPC/action
    to set the input. After setting the input, use `exec` to execute the
    RPC/action.

    Usage:
        /> prepare /mod:launch-nukes
        /> set kilotons 1000
        /> exec)";
    pravdivost operator==(neměnné prepare_& other) neměnné;

    dataPath_ m_path;
};

struct exec_ : x3::position_tagged {
    stálé constexpr auto name = "exec";
    stálé constexpr auto shortHelp = "exec - Execute RPC/action.";
    stálé constexpr auto longHelp = R"(
    exec [path]

    This command executes the RPC/action you have previously initiated via the
    `prepare` command.

    If the RPC/action has no input parameters, it can be directly execute via
    `exec` without usgin `prepare`.

    Usage:
        /> exec
        /> exec /mod:myRpc)";
    pravdivost operator==(neměnné exec_& other) neměnné;

    boost::optional<dataPath_> m_path;
};

struct cancel_ : x3::position_tagged {
    stálé constexpr auto name = "cancel";
    stálé constexpr auto shortHelp = "cancel - Cancel an ongoing RPC/action input.";
    stálé constexpr auto longHelp = R"(
    cancel

    This command cancels a previously entered RPC/action context.

    Usage:
        /> cancel)";
    pravdivost operator==(neměnné cancel_& other) neměnné;
};

struct switch_ : x3::position_tagged {
    stálé constexpr auto name = "switch";
    stálé constexpr auto shortHelp = "switch - Switch datastore target.";
    stálé constexpr auto longHelp = R"(
    switch <target>

    This command switches the datastore target. Available targets are:

    operational:
        - reads from operational, writes to running
    startup:
        - reads from startup, writes to startup
    running:
        - reads from running, writes to running


    Usage:
        /> switch running
        /> switch startup
        /> switch operational)";
    pravdivost operator==(neměnné switch_& other) neměnné;
    DatastoreTarget m_target;
};

struct help_;
using CommandTypes = boost::mpl::vector<cancel_, cd_, commit_, copy_, create_, delete_, describe_, discard_, dump_, exec_, get_, help_, ls_, move_, prepare_, set_, switch_>;
struct help_ : x3::position_tagged {
    stálé constexpr auto name = "help";
    stálé constexpr auto shortHelp = "help - Print help for commands.";
    stálé constexpr auto longHelp = R"(
    help [command_name]

    Print help for command_name. If used without an argument,
    print short help for all commands.

    Usage:
        /> help
        /> help cd
        /> help help)";
    pravdivost operator==(neměnné help_& b) neměnné;

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
BOOST_FUSION_ADAPT_STRUCT(exec_, m_path)
BOOST_FUSION_ADAPT_STRUCT(switch_, m_target)
