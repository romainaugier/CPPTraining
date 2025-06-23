#include <vector>
#include <string>
#include <iostream>
#include <cstdint>
#include <stack>
#include <format>
#include <cctype>
#include <tuple>

static std::ostream_iterator<char> STDOUT(std::cout);
static std::ostream_iterator<char> STDERR(std::cerr);

/*
    https://dl.acm.org/doi/pdf/10.1145/363347.363387
*/

enum RegexOperatorType : uint16_t
{
    RegexOperatorType_Alternate,
    RegexOperatorType_Concatenate,
    RegexOperatorType_ZeroOrMore,
    RegexOperatorType_OneOrMore,
    RegexOperatorType_ZeroOrOne,
};

const char* regex_operator_type_to_string(uint16_t op)
{
    switch(op)
    {
        case RegexOperatorType_Alternate:
            return "Alternate";
        case RegexOperatorType_Concatenate:
            return "Concatenate";
        case RegexOperatorType_ZeroOrMore:
            return "ZeroOrMore";
        case RegexOperatorType_OneOrMore:
            return "OneOrMore";
        case RegexOperatorType_ZeroOrOne:
            return "ZeroOrOne";
        default:
            return "Unknown Operator";
    }
}

enum RegexCharacterType : uint16_t
{
    RegexCharacterType_Single,
    RegexCharacterType_Range,
    RegexCharacterType_Any,
};

const char* regex_character_type_to_string(uint16_t c)
{
    switch(c)
    {
        case RegexCharacterType_Single:
            return "Single";
        case RegexCharacterType_Range:
            return "Range";
        case RegexCharacterType_Any:
            return "Any";
        default:
            return "Unknown Character";
    }
}

/* Lexing / Parsing */

enum RegexTokenType : uint32_t
{
    RegexTokenType_Character,
    RegexTokenType_CharacterRange,
    RegexTokenType_Operator,
    RegexTokenType_GroupBegin,
    RegexTokenType_GroupEnd,
    RegexTokenType_Invalid,
};

/* We don't use string_view because we want to keep this struct 16 bytes */
struct RegexToken
{
    const char* data;
    uint32_t data_len;
    uint16_t type;
    uint16_t encoding; /* Either RegexOperatorType or RegexCharacterType */

    RegexToken() : data(nullptr), data_len(0), type(RegexTokenType_Invalid), encoding(0) {}

    RegexToken(const char* data, uint32_t data_len, uint32_t type, uint16_t encoding = 0) : data(data), 
                                                                                            data_len(data_len),
                                                                                            type(type),
                                                                                            encoding(encoding) {}

    RegexToken(const RegexToken& other) : data(other.data),
                                          data_len(other.data_len),
                                          type(other.type),
                                          encoding(other.encoding) {}

    RegexToken& operator=(const RegexToken& other) 
    {
        this->data = other.data;
        this->data_len = other.data_len;
        this->type = other.type;
        this->encoding = other.encoding;

        return *this;
    }

    RegexToken(RegexToken&& other) noexcept : data(other.data),
                                              data_len(other.data_len),
                                              type(other.type),
                                              encoding(other.encoding) {}

    RegexToken& operator=(RegexToken&& other) noexcept
    {
        this->data = other.data;
        this->data_len = other.data_len;
        this->type = other.type;
        this->encoding = other.encoding;

        return *this;
    }

    void print() const
    {
        switch(type)
        {
            case RegexTokenType_Character:
            {
                if(this->data != nullptr && this->data_len > 0)
                {
                    std::format_to(STDOUT,
                                   "CHAR({}, {})",
                                   std::string_view(this->data, this->data_len),
                                   regex_character_type_to_string(this->encoding));
                }
                else
                {
                    std::format_to(STDOUT,
                                   "CHAR({})",
                                   regex_character_type_to_string(this->encoding));

                }

                break;
            }

            case RegexTokenType_CharacterRange:
            {
                std::format_to(STDOUT, "RANGE({})", std::string_view(this->data, this->data_len));
                break;
            }

            case RegexTokenType_Operator:
            {
                std::format_to(STDOUT, "OP({})", regex_operator_type_to_string(this->encoding));
                break;
            }

            case RegexTokenType_GroupBegin:
            {
                std::format_to(STDOUT, "GROUP_BEGIN({})", this->encoding);
                break;
            }

            case RegexTokenType_GroupEnd:
            {
                std::format_to(STDOUT, "GROUP_END({})", this->encoding);
                break;
            }

            case RegexTokenType_Invalid:
            {
                std::format_to(STDOUT, "INVALID");
                break;
            }

            default:
            {
                std::format_to(STDOUT, "UNKNOWN_TOKEN");
                break;
            }
        }
    }
};

std::tuple<bool, std::vector<RegexToken>> lex_regex(const std::string& regex) noexcept
{
    std::vector<RegexToken> tokens;

    uint32_t i = 0;
    bool need_concat = false;

    while(i < regex.size())
    {
        if(std::isalnum(regex[i]))
        {
            if(need_concat)
            {
                tokens.emplace_back(nullptr, 0u, RegexTokenType_Operator, RegexOperatorType_Concatenate);
            }

            tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_Character, RegexCharacterType_Single);
            need_concat = true;
        }
        else
        {
            switch(regex[i])
            {
                case '|':
                {
                    tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_Operator, RegexOperatorType_Alternate);
                    need_concat = false;
                    break;
                }

                case '*':
                {
                    tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_Operator, RegexOperatorType_ZeroOrMore);
                    break;
                }

                case '+':
                {
                    tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_Operator, RegexOperatorType_OneOrMore);
                    break;
                }

                case '?':
                {
                    tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_Operator, RegexOperatorType_ZeroOrOne);
                    break;
                }

                case '.':
                {
                    if(need_concat)
                    {
                        tokens.emplace_back(nullptr, 0u, RegexTokenType_Operator, RegexOperatorType_Concatenate);
                    }

                    tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_Character, RegexCharacterType_Any);
                    need_concat = true;

                    break;
                }

                case '(':
                {
                    if(need_concat)
                    {
                        tokens.emplace_back(nullptr, 0u, RegexTokenType_Operator, RegexOperatorType_Concatenate);
                    }

                    tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_GroupBegin);
                    need_concat = false;

                    break;
                }

                case ')':
                {
                    tokens.emplace_back(regex.data() + i, 1u, RegexTokenType_GroupEnd);
                    need_concat = true;
                    break;
                }

                case '[':
                {
                    if(need_concat)
                    {
                        tokens.emplace_back(nullptr, 0u, RegexTokenType_Operator, RegexOperatorType_Concatenate);
                    }

                    const char* start = regex.data() + ++i;

                    while(i < regex.size() && regex[i] != ']')
                    {
                        i++;
                    }

                    if(i >= regex.size())
                    {
                        std::format_to(STDERR, "Unclosed character range in regular expression\n");
                        return std::make_tuple(false, tokens);
                    }

                    tokens.emplace_back(start, (regex.data() + i) - start, RegexTokenType_CharacterRange);
                    need_concat = true;
                    break;
                }

                default:
                {
                    std::format_to(STDERR, "Unsupported character found in regular expression: {}\n", regex[i]);
                    return std::make_tuple(false, tokens);
                }
            }
        }

        i++;
    }

    return std::make_tuple(true, tokens);
}

/* Operator precedence for infix to postfix conversion */
int get_operator_precedence(RegexOperatorType op)
{
    switch(op)
    {
        case RegexOperatorType_Alternate:
            return 1;
        case RegexOperatorType_Concatenate:
            return 2;
        case RegexOperatorType_ZeroOrMore:
        case RegexOperatorType_OneOrMore:
        case RegexOperatorType_ZeroOrOne:
            return 3;
        default:
            return 0;
    }
}

bool is_left_associative(RegexOperatorType op)
{
    switch(op)
    {
        case RegexOperatorType_Alternate:
        case RegexOperatorType_Concatenate:
            return true;
        case RegexOperatorType_ZeroOrMore:
        case RegexOperatorType_OneOrMore:
        case RegexOperatorType_ZeroOrOne:
            return false; /* These are postfix operators */
        default:
            return true;
    }
}

/* Infix to postfix using Shunting Yard algorithm */
std::tuple<bool, std::vector<RegexToken>> parse_regex(const std::vector<RegexToken>& tokens) noexcept
{
    std::vector<RegexToken> output;
    std::stack<RegexToken> operators;

    uint32_t current_group_id = 0;

    for(const auto& token : tokens)
    {
        switch(token.type)
        {
            case RegexTokenType_Character:
            case RegexTokenType_CharacterRange:
            {
                output.push_back(token);
                break;
            }

            case RegexTokenType_Operator:
            {
                RegexOperatorType current_op = static_cast<RegexOperatorType>(token.encoding);
                int current_precedence = get_operator_precedence(current_op);

                while(!operators.empty() && 
                      operators.top().type == RegexTokenType_Operator)
                {
                    RegexOperatorType stack_op = static_cast<RegexOperatorType>(operators.top().encoding);
                    int stack_precedence = get_operator_precedence(stack_op);

                    if(stack_precedence > current_precedence ||
                       (stack_precedence == current_precedence && is_left_associative(current_op)))
                    {
                        output.push_back(operators.top());
                        operators.pop();
                    }
                    else
                    {
                        break;
                    }
                }

                operators.push(token);
                break;
            }

            case RegexTokenType_GroupBegin:
            {
                operators.push(token);

                RegexToken group_begin_token = token;
                group_begin_token.encoding = current_group_id++;

                output.push_back(group_begin_token);

                break;
            }

            case RegexTokenType_GroupEnd:
            {
                while(!operators.empty() && operators.top().type != RegexTokenType_GroupBegin)
                {
                    output.push_back(operators.top());
                    operators.pop();
                }

                if(operators.empty())
                {
                    std::format_to(STDERR, "Mismatched parentheses in regular expression\n");
                    return std::make_tuple(false, output);
                }

                RegexToken group_end_token = token;
                group_end_token.encoding = --current_group_id;

                output.push_back(group_end_token);

                operators.pop();

                break;
            }

            default:
            {
                std::format_to(STDERR, "Invalid token type during parsing\n");
                return std::make_tuple(false, output);
            }
        }
    }

    while(!operators.empty())
    {
        if(operators.top().type == RegexTokenType_GroupBegin)
        {
            std::format_to(STDERR, "Mismatched parentheses in regular expression\n");
            return std::make_tuple(false, output);
        }

        output.push_back(operators.top());
        operators.pop();
    }

    return std::make_tuple(true, output);
}

void print_tokens(const std::vector<RegexToken>& tokens)
{
    std::format_to(STDOUT, "Tokens: ");

    for(size_t i = 0; i < tokens.size(); ++i)
    {
        if(i > 0)
        {
            std::format_to(STDOUT, " ");
        }

        tokens[i].print();
    }

    std::format_to(STDOUT, "\n");
}

/* Instructions / Bytecode */

enum RegexInstrOpCode : uint8_t
{
    RegexInstrType_TestSingle,
    RegexInstrType_TestRange,
    RegexInstrType_TestNegatedRange,
    RegexInstrType_TestAny,
    RegexInstrType_Split,
    RegexInstrType_Split32,
    RegexInstrType_Jump,
    RegexInstrType_Jump32,
    RegexInstrType_Match,
    RegexInstrType_Fail,
    RegexInstrType_GroupStart,
    RegexInstrType_GroupEnd,
};

struct RegexThread
{
    size_t pc;
    size_t sp;

    std::vector<std::pair<size_t, size_t>> groups;

    RegexThread(size_t pc, size_t sp) : pc(pc), sp(sp) {}
};


class Regex
{
    using RegexByteCode = std::vector<std::byte>;
    using RegexThreads = std::vector<RegexThread>;

    RegexByteCode _bytecode;

    bool _compile(const std::string& regex) noexcept
    {
        this->_bytecode.clear();

        auto [lex_success, tokens] = lex_regex(regex);

        if(!lex_success)
        {
            std::format_to(STDERR, "Failed to lex regex: {}\n", regex);
            return false;
        }

        auto [parse_success, postfix_tokens] = parse_regex(tokens);

        if(!parse_success)
        {
            std::format_to(STDERR, "Failed to parse regex: {}\n", regex);
            return false;
        }

        return true;
    }

public:
    Regex(const std::string& regex)
    {
        this->_compile(regex);
    }

    bool match(const std::string& str) const noexcept
    {
        if(this->_bytecode.size() == 0)
        {
            return false;
        }

        RegexThreads threads;

        return true;
    }
};

int main(int argc, char** argv) 
{
    Regex regex1("[0-9]*");
    
    Regex regex2("a+b|c");
    
    Regex regex3("(a|b)*c");

    return 0;
}