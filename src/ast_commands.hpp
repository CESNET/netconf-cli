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
#include "datastore_access.hpp"
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

    Lists available child nodes in the current context node. Optionally accepts
    a path argument. Accepts both schema paths and data paths. Path starting
    with a forward slash means an absolute path.

    Options:
        --recursive    makes `ls` work recursively

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
    cd <path>

    Changes context to a node specified by <path>. Only accepts data paths
    (paths with all list keys supplied).

    Usage:
        /> cd /module:node/node2
        /> cd ..)";
    bool operator==(const cd_& b) const;
    dataPath_ m_path;
};

struct create_ : x3::position_tagged {
    static constexpr auto name = "create";
    static constexpr auto shortHelp = "create - Create a node.";
    static constexpr auto longHelp = R"(
    create <path>

    Creates a node specified by <path>.
    Supported node types are list items, leaflist instance and presence
    containers.

    Usage:
        /> create /module:pContainer
        /> create /module:leafList['value']
        /> create /module:list[key=value][anotherKey=value])";
    bool operator==(const create_& b) const;
    dataPath_ m_path;
};

struct delete_ : x3::position_tagged {
    static constexpr auto name = "delete";
    static constexpr auto shortHelp = "delete - Delete a node.";
    static constexpr auto longHelp = R"(
    delete <path>

    Deletes a node specified by <path>.
    Supported node types are leafs, list items, leaflist instance and presence
    containers.

    Usage:
        /> delete /module:pContainer
        /> delete /module:leafList['value']
        /> delete /module:list[key=value][anotherKey=value])";
    bool operator==(const delete_& b) const;
    dataPath_ m_path;
};

struct set_ : x3::position_tagged {
    static constexpr auto name = "set";
    static constexpr auto shortHelp = "set - Set value of a leaf.";
    static constexpr auto longHelp = R"(
    set <path_to_leaf> <value>

    Sets the leaf specified by <path_to_leaf> to <value>.
    Values of type string must be enclosed in quotation marks (" or ').

    Usage:
        /> set /module:someNumber 123
        /> set /module:someString 'abc'
        /> set /module:someString "abc")";
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

    Prints out content of the current datastore at a specified [path], or below
    the current context tree.

    Usage:
        /> get
        /> get /module:path)";
    bool operator==(const get_& b) const;
    boost::optional<DatastoreTarget> m_dsTarget;
    boost::optional<boost::variant<dataPath_, module_>> m_path;
};

struct describe_ : x3::position_tagged {
    static constexpr auto name = "describe";
    static constexpr auto shortHelp = "describe - Print information about YANG tree path.";
    static constexpr auto longHelp = R"(
    describe <path>

    Shows documentation of YANG tree paths. In the YANG model, each item may
    have an optional `description` which often explains the function of that
    node to the end user. This command takes the description from the YANG model
    and shows it to the user along with additional data, such as the type of the
    node, units of leaf values, etc.

    Usage:
        /> describe /module:node)";
    bool operator==(const describe_& b) const;

    boost::variant<schemaPath_, dataPath_> m_path;
};

struct copy_ : x3::position_tagged {
    static constexpr auto name = "copy";
    static constexpr auto shortHelp = "copy - Copy configuration.";
    static constexpr auto longHelp = R"(
    copy <source> <destination>

    Copies configuration from <source> to <destination>.

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
    static constexpr auto shortHelp = "move - Move (leaf)list instances.";
    static constexpr auto longHelp = R"(
    move <path> begin
    move <path> end
    move <path> before <key>
    move <path> after <key>

    Moves the instance specified by <path> to the position
    specified by the second and third argument.

    Usage:
        /> move mod:leaflist['abc'] begin
        /> move mod:leaflist['def'] after 'abc'
        /> move mod:interfaces[name='eth0'] after [name='eth1'])";
    bool operator==(const move_& b) const;

    dataPath_ m_source;

    std::variant<yang::move::Absolute, yang::move::Relative> m_destination;
};

struct dump_ : x3::position_tagged {
    static constexpr auto name = "dump";
    static constexpr auto shortHelp = "dump - Print out datastore content as JSON or XML.";
    static constexpr auto longHelp = R"(
    dump xml|json

    Prints out the content of the datastore. Supports JSON and XML.

    Usage:
        /> dump xml
        /> dump json)";
    bool operator==(const dump_& other) const;

    DataFormat m_format;
};

struct prepare_ : x3::position_tagged {
    static constexpr auto name = "prepare";
    static constexpr auto shortHelp = "prepare - Initiate RPC/action.";
    static constexpr auto longHelp = R"(
    prepare <path-to-rpc-or-action>

    This command enters a mode for entering input parameters for the RPC/action.
    In this mode, you can use commands like `set` on nodes inside the RPC/action
    to set the input. After setting the input, use `exec` to execute the
    RPC/action.

    Usage:
        /> prepare /mod:launch-nukes
        /> set kilotons 1000
        /> exec)";
    bool operator==(const prepare_& other) const;

    dataPath_ m_path;
};

struct exec_ : x3::position_tagged {
    static constexpr auto name = "exec";
    static constexpr auto shortHelp = "exec - Execute RPC/action.";
    static constexpr auto longHelp = R"(
    exec [path]

    This command executes the RPC/action you have previously initiated via the
    `prepare` command.

    If the RPC/action has no input parameters, it can be directly execute via
    `exec` without usgin `prepare`.

    Usage:
        /> exec
        /> exec /mod:myRpc)";
    bool operator==(const exec_& other) const;

    boost::optional<dataPath_> m_path;
};

struct cancel_ : x3::position_tagged {
    static constexpr auto name = "cancel";
    static constexpr auto shortHelp = "cancel - Cancel an ongoing RPC/action input.";
    static constexpr auto longHelp = R"(
    cancel

    This command cancels a previously entered RPC/action context.

    Usage:
        /> cancel)";
    bool operator==(const cancel_& other) const;
};

struct switch_ : x3::position_tagged {
    static constexpr auto name = "switch";
    static constexpr auto shortHelp = "switch - Switch datastore target.";
    static constexpr auto longHelp = R"(
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
    bool operator==(const switch_& other) const;
    DatastoreTarget m_target;
};

struct quit_ : x3::position_tagged {
    static constexpr auto name = "quit";
    static constexpr auto shortHelp = "quit - Quit the console.";
    static constexpr auto longHelp = R"(
    quit

    Quit the console. Accepts no arguments.

    Usage:
        /> quit)";
    bool operator==(const quit_& b) const;
};

struct help_;
using CommandTypes = boost::mpl::vector<cancel_, cd_, commit_, copy_, create_, delete_, describe_, discard_, dump_, exec_, get_, help_, ls_, move_, prepare_, quit_, set_, switch_>;
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
BOOST_FUSION_ADAPT_STRUCT(get_, m_dsTarget, m_path)
BOOST_FUSION_ADAPT_STRUCT(copy_, m_source, m_destination)
BOOST_FUSION_ADAPT_STRUCT(move_, m_source, m_destination)
BOOST_FUSION_ADAPT_STRUCT(dump_, m_format)
BOOST_FUSION_ADAPT_STRUCT(prepare_, m_path)
BOOST_FUSION_ADAPT_STRUCT(exec_, m_path)
BOOST_FUSION_ADAPT_STRUCT(switch_, m_target)
BOOST_FUSION_ADAPT_STRUCT(quit_)
