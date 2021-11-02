#define BOOST_SPIRIT_X3_DEBUG
#include <iostream>
#include "datastore_access.hpp"
#include "ast_commands.hpp"

std::ostream& operator<<(std::ostream& os, DatastoreTarget lol)
{
    os << "DatastoreTarget{" <<(int)lol<< "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, special_ lol)
{
    os << "special_{" <<(int)lol.m_value<< "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, empty_)
{
    os << "empty_{}";
    return os;
}

std::ostream& operator<<(std::ostream& os, cancel_)
{
    os << "cancel_{}";
    return os;
}

std::ostream& operator<<(std::ostream& os, LsOption lol)
{
    os << "LsOption{" <<(int)lol<< "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, Datastore lol)
{
    os << "Datastore{" <<(int)lol<< "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, DataFormat lol)
{
    os << "DataFormat{" <<(int)lol<< "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, Scope lol)
{
    os << "Scope{" <<(int)lol<< "}";
    return os;
}

template <typename Type>
std::ostream& operator<<(std::ostream& os, boost::type<Type>)
{
    os << "boost::type<Type>{}";
    return os;
}

namespace std {
std::ostream& operator<<(std::ostream& os, std::variant<yang::move::Absolute, yang::move::Relative>)
{
    os << "variant_move{}";
    return os;
}

std::ostream& operator<<(std::ostream& os, std::variant<container_, listElement_, nodeup_, leaf_, leafListElement_, leafList_, list_, rpcNode_, actionNode_>)
{
    os << "variant_of_data_nodes{}";
    return os;
}


std::ostream& operator<<(std::ostream& os, std::variant<container_, list_, nodeup_, leaf_, leafList_, rpcNode_, actionNode_>)
{
    os << "variant_of_schema_nodes{}";
    return os;
}

}
