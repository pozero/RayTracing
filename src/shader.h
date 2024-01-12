#pragma once

#include <string>
#include <vector>

class Shader {
public:
    Shader(std::string const &path, unsigned int type);

    ~Shader();

private:
    friend class Program;
    unsigned int m_id;
};

class Program {
public:
    Program(Shader const &vert, Shader const &frag);

    Program(std::string const &path);

    ~Program();

    void use() const;

    void setBool(const std::string &name, bool value) const;

    void setInt(const std::string &name, int value) const;

    void setFloat(const std::string &name, float value) const;

private:
    void check_link_error() const;

    unsigned int m_id;
};
