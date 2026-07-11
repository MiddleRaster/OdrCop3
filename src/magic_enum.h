#pragma once
// a stripped-down version of magic_enum

#include <array>

template <auto V> constexpr const char* raw_name() noexcept { return __FUNCSIG__; }
template <auto V> std::string enum_name_of()
{
    std::string s = raw_name<V>();
    auto end = s.rfind('>');
    auto start = s.rfind('<', end) + 1;
    return s.substr(start, end - start);
}
template <auto V> bool is_valid() { return enum_name_of<V>()[0] != '('; }
template <typename E, int Min, int Max> struct EnumTable
{
    static constexpr int     count = Max - Min + 1;
    std::array<std::string, count> names{};

    EnumTable() { fill(std::make_integer_sequence<int, count>{}); }
    template <int... Is> void fill(std::integer_sequence<int, Is...>)
    {
        ((names[Is] = is_valid<static_cast<E>(Min + Is)>() ? enum_name_of<static_cast<E>(Min + Is)>() : std::string{}), ...);
    }
    std::string lookup(int v) const
    {
        if (v < Min || v > Max) return {};
        return names[v - Min];
    }
};
template <typename E, int Min = 0, int Max = 127> std::string enum_name(E value)
{
    static const EnumTable<E, Min, Max> table;
    std::string name = table.lookup(static_cast<int>(value));
    if (name.empty())
        return "unknown(" + std::to_string(static_cast<int>(value)) + ")";
    return name;
}
