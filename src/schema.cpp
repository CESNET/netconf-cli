/*
 * Copyright (C) 2018 CESNET, https://photonics.cesnet.cz/
 * Copyright (C) 2018 FIT CVUT, https://fit.cvut.cz/
 *
 * Written by Václav Kubernát <kubervac@fit.cvut.cz>
 *
*/

#include "schema.hpp"

Schema::~Schema() = default;

bool Schema::isList(const schemaPath_& location, const ModuleNodePair& node) const
{
    try {
        return nodeType(location, node) == yang::NodeTypes::List;
    } catch (InvalidNodeException&) {
        return false;
    }
}

bool Schema::isPresenceContainer(const schemaPath_& location, const ModuleNodePair& node) const
{
    try {
        return nodeType(location, node) == yang::NodeTypes::PresenceContainer;
    } catch (InvalidNodeException&) {
        return false;
    }
}

bool Schema::isPresenceContainer(const schemaPath_& path) const
{
    try {
        return nodeType(path) == yang::NodeTypes::PresenceContainer;
    } catch (InvalidNodeException&) {
        return false;
    }
}

bool Schema::isContainer(const schemaPath_& location, const ModuleNodePair& node) const
{
    try {
        auto type = nodeType(location, node);
        return type == yang::NodeTypes::Container || type == yang::NodeTypes::PresenceContainer;
    } catch (InvalidNodeException&) {
        return false;
    }
}

bool Schema::isLeaf(const schemaPath_& location, const ModuleNodePair& node) const
{
    try {
        return nodeType(location, node) == yang::NodeTypes::Leaf;
    } catch (InvalidNodeException&) {
        return false;
    }
}

bool Schema::isLeafList(const std::string& path) const
{
    try {
        return nodeType(path) == yang::NodeTypes::LeafList;
    } catch (InvalidNodeException&) {
        return false;
    }
}
