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

enum RegexOperatorType : std::uint16_t
{
    RegexOperatorType_Alternate,
    RegexOperatorType_Concatenate,
    RegexOperatorType_ZeroOrMore,
    RegexOperatorType_OneOrMore,
    RegexOperatorType_ZeroOrOne,
};

const char* regex_operator_type_to_string(std::uint16_t op)
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

enum RegexCharacterType : std::uint16_t
{
    RegexCharacterType_Single,
    RegexCharacterType_Range,
    RegexCharacterType_Any,
};

const char* regex_character_type_to_string(std::uint16_t c)
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

enum RegexTokenType : std::uint32_t
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
    std::uint32_t data_len;
    std::uint16_t type;
    std::uint16_t encoding; /* Either RegexOperatorType or RegexCharacterType */

    RegexToken() : data(nullptr), data_len(0), type(RegexTokenType_Invalid), encoding(0) {}

    RegexToken(const char* data,
               std::uint32_t data_len,
               std::uint32_t type,
               std::uint16_t encoding = 0) : data(data), 
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

    std::uint32_t i = 0;
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

    std::uint32_t current_group_id = 0;

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

    for(std::size_t i = 0; i < tokens.size(); ++i)
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

using RegexByteCode = std::vector<std::byte>;

enum RegexInstrOpCode : std::uint8_t
{
    RegexInstrOpCode_TestSingle,
    RegexInstrOpCode_TestRange,
    RegexInstrOpCode_TestNegatedRange,
    RegexInstrOpCode_TestAny,
    RegexInstrOpCode_JumpEq, /* jumps if status_flag = true */
    RegexInstrOpCode_JumpNeq, /* jumps if status_flag = false */
    RegexInstrOpCode_Accept,
    RegexInstrOpCode_Fail,
    RegexInstrOpCode_GroupStart,
    RegexInstrOpCode_GroupEnd,
    RegexInstrOpCode_PushState,
    RegexInstrOpCode_PopState,
};

enum RegexInstrOpType : std::uint32_t
{
    RegexInstrOpType_TestOp,
    RegexInstrOpType_UnaryOp,
    RegexInstrOpType_BinaryOp,
    RegexInstrOpType_GroupOp,
};

/* std::bitcast like functions to convert back and forth */
template<typename T,
         typename = std::enable_if<sizeof(T) == sizeof(std::byte)>>
inline std::byte BYTE(const T& t) noexcept { std::byte b; std::memcpy(&b, &t, sizeof(std::byte)); return b; }

template<typename T,
         typename = std::enable_if<sizeof(T) == sizeof(std::byte)>>
inline std::byte BYTE(T&& t) noexcept { std::byte b; const T t2 = t; std::memcpy(&b, &t2, sizeof(std::byte)); return b; }

inline char CHAR(const std::byte b) noexcept { char c; std::memcpy(&c, &b, sizeof(char)); return c; }

static constexpr int JUMP_FAIL = std::numeric_limits<int>::max();

inline std::tuple<std::byte, std::byte, std::byte, std::byte> encode_jump(const int jump_size) noexcept
{
    return std::make_tuple(BYTE((jump_size >> 24) & 0xFF),
                           BYTE((jump_size >> 16) & 0xFF),
                           BYTE((jump_size >> 8) & 0xFF),
                           BYTE((jump_size >> 0) & 0xFF));
}

inline int decode_jump(const std::byte* bytecode) noexcept
{
    return static_cast<int>(bytecode[0]) << 24 |
           static_cast<int>(bytecode[1]) << 16 |
           static_cast<int>(bytecode[2]) << 8 |
           static_cast<int>(bytecode[3]) << 0;
}

inline std::size_t emit_jump(RegexByteCode& code, RegexInstrOpCode op) noexcept
{
    code.push_back(BYTE(op));

    std::size_t pos = code.size();
    const auto [b1, b2, b3, b4] = encode_jump(0);

    code.insert(code.end(), { b1, b2, b3, b4 });

    return pos;
}

inline void patch_jump(RegexByteCode& code, std::size_t jump_pos, int offset) noexcept
{
    const auto [b1, b2, b3, b4] = encode_jump(offset);

    code[jump_pos + 0] = b1;
    code[jump_pos + 1] = b2;
    code[jump_pos + 2] = b3;
    code[jump_pos + 3] = b4;
}


using Fragment = std::pair<std::uint32_t, RegexByteCode>;

std::tuple<bool, RegexByteCode> regex_emit(const std::vector<RegexToken>& tokens) noexcept
{
    std::stack<Fragment> fragments;

    for(const auto& token : tokens)
    {
        switch(token.type)
        {
            case RegexTokenType_Character:
                switch(token.encoding)
                {
                    case RegexCharacterType_Single:
                        fragments.push({ RegexInstrOpType_TestOp, 
                            { 
                                BYTE(RegexInstrOpCode_TestSingle),
                                BYTE(token.data[0]) 
                            }
                        });
                        break;

                    case RegexCharacterType_Any:
                        fragments.push({ RegexInstrOpType_TestOp,
                            {
                                BYTE(RegexInstrOpCode_TestAny)
                            }
                        });
                        break;
                }

                break;

            case RegexTokenType_CharacterRange:
                fragments.push({ RegexInstrOpType_TestOp,
                    { 
                        BYTE(RegexInstrOpCode_TestRange),
                        BYTE(token.data[0]),
                        BYTE(token.data[2]) 
                    }
                });
                break;

            case RegexTokenType_Operator:
                switch(token.encoding)
                {
                    case RegexOperatorType_Alternate:
                    {
                        RegexByteCode frag;

                        const auto [rhs_op_type, rhs] = std::move(fragments.top());
                        fragments.pop();
                        const auto [lhs_op_type, lhs] = std::move(fragments.top());
                        fragments.pop();

                        frag.insert(frag.end(), lhs.begin(), lhs.end());

                        frag.push_back(BYTE(RegexInstrOpCode_JumpEq));

                        auto [lb1, lb2, lb3, lb4] = encode_jump(rhs.size() + 1);
                        frag.insert(frag.end(), { lb1, lb2, lb3, lb4 });

                        frag.insert(frag.end(), rhs.begin(), rhs.end());

                        fragments.push({ RegexInstrOpType_BinaryOp, std::move(frag) });

                        break;
                    }

                    case RegexOperatorType_Concatenate:
                    {
                        RegexByteCode frag;

                        const auto [rhs_op_type, rhs] = std::move(fragments.top());
                        fragments.pop();
                        const auto [lhs_op_type, lhs] = std::move(fragments.top());
                        fragments.pop();

                        frag.insert(frag.end(), lhs.begin(), lhs.end());

                        if(lhs_op_type == RegexInstrOpType_TestOp)
                        {
                            frag.push_back(BYTE(RegexInstrOpCode_JumpNeq));

                            auto [lb1, lb2, lb3, lb4] = encode_jump(JUMP_FAIL);
                            frag.insert(frag.end(), { lb1, lb2, lb3, lb4 });
                        }

                        frag.insert(frag.end(), rhs.begin(), rhs.end());

                        if(rhs_op_type == RegexInstrOpType_TestOp)
                        {
                            frag.push_back(BYTE(RegexInstrOpCode_JumpNeq));

                            auto [rb1, rb2, rb3, rb4] = encode_jump(JUMP_FAIL);
                            frag.insert(frag.end(), { rb1, rb2, rb3, rb4 });
                        }

                        fragments.push({ RegexInstrOpType_BinaryOp, std::move(frag) });

                        break;
                    }

                    case RegexOperatorType_ZeroOrMore:
                    {
                        RegexByteCode frag;

                        const auto [op_type, body] = std::move(fragments.top());
                        fragments.pop();

                        frag.push_back(BYTE(RegexInstrOpCode_PushState));

                        std::size_t loop_start = frag.size();

                        frag.insert(frag.end(), body.begin(), body.end());

                        std::size_t exit_jump_pos = emit_jump(frag, RegexInstrOpCode_JumpNeq);

                        frag.push_back(BYTE(RegexInstrOpCode_PushState));
                        std::size_t loop_jump_pos = emit_jump(frag, RegexInstrOpCode_JumpEq);

                        int offset_back = static_cast<int>(loop_start - (loop_jump_pos + 4));
                        patch_jump(frag, loop_jump_pos, offset_back);

                        int offset_exit = static_cast<int>(frag.size() - (exit_jump_pos + 4));
                        patch_jump(frag, exit_jump_pos, offset_exit);

                        fragments.push({ RegexInstrOpType_UnaryOp, std::move(frag) });
                    }

                    case RegexOperatorType_OneOrMore:
                    {
                        break;
                    }

                    case RegexOperatorType_ZeroOrOne:
                    {
                        break;
                    }

                }
        }
    }

    auto [_, bytecode] = std::move(fragments.top());
    fragments.pop();

    bytecode.push_back(BYTE(RegexInstrOpCode_Accept));
    bytecode.push_back(BYTE(RegexInstrOpCode_Fail));

    std::size_t i = 0;

    while(i < bytecode.size())
    {
        switch(static_cast<std::uint8_t>(bytecode[i]))
        {
            case RegexInstrOpCode_JumpEq:
            case RegexInstrOpCode_JumpNeq:
            {
                int offset = decode_jump(std::addressof(bytecode[i + 1]));

                if(offset == JUMP_FAIL)
                {
                    offset = static_cast<int>(bytecode.size() - i);

                    const auto [b1, b2, b3, b4] = encode_jump(offset);
                    bytecode[i + 1] = b1;
                    bytecode[i + 2] = b2;
                    bytecode[i + 3] = b3;
                    bytecode[i + 4] = b4;
                }

                i += 5;

                break;
            }

            case RegexInstrOpCode_GroupStart:
            case RegexInstrOpCode_GroupEnd:
            case RegexInstrOpCode_Accept:
            case RegexInstrOpCode_Fail:
            case RegexInstrOpCode_PushState:
            case RegexInstrOpCode_PopState:
            case RegexInstrOpCode_TestAny:
                i++;
                break;

            case RegexInstrOpCode_TestSingle:
                i += 2;
                break;

            case RegexInstrOpCode_TestRange:
            case RegexInstrOpCode_TestNegatedRange:
                i += 3;
                break;

            default:
                i++;
                break;
        }
    }

    return make_tuple(true, bytecode);
}

void regex_disasm(const RegexByteCode& bytecode) noexcept
{
    std::size_t i = 0;

    while(i < bytecode.size())
    {
        switch(static_cast<std::uint8_t>(bytecode[i]))
        {
            case RegexInstrOpCode_TestSingle:
                std::format_to(STDOUT,
                               "TESTSINGLE {}",
                               CHAR(bytecode[i + 1]));
                i += 2;
                break;
            case RegexInstrOpCode_TestRange:
                std::format_to(STDOUT,
                               "TESTRANGE {}-{}",
                               CHAR(bytecode[i + 1]),
                               CHAR(bytecode[i + 2]));
                i += 3;
                break;
            case RegexInstrOpCode_TestNegatedRange:
                std::format_to(STDOUT,
                               "TESTNEGRANGE {}-{}",
                               CHAR(bytecode[i + 1]),
                               CHAR(bytecode[i + 2]));
                i += 3;
                break;
            case RegexInstrOpCode_TestAny:
                std::format_to(STDOUT, "TESTANY");
                i++;
                break;
            case RegexInstrOpCode_JumpEq:
            case RegexInstrOpCode_JumpNeq:
                std::format_to(STDOUT,
                               "{} {}{}",
                               static_cast<std::uint8_t>(bytecode[i]) == RegexInstrOpCode_JumpEq ? "JUMPEQ" : "JUMPNEQ",
                               decode_jump(std::addressof(bytecode[i + 1])) >= 0 ? "+" : "",
                               decode_jump(std::addressof(bytecode[i + 1])));
                i += 5;
                break;
            case RegexInstrOpCode_Accept:
                std::format_to(STDOUT, "ACCEPT");
                i++;
                break;
            case RegexInstrOpCode_Fail:
                std::format_to(STDOUT, "FAIL");
                i++;
                break;
            case RegexInstrOpCode_GroupStart:
                std::format_to(STDOUT, "GROUPSTART");
                i++;
                break;
            case RegexInstrOpCode_GroupEnd:
                std::format_to(STDOUT, "GROUPEND");
                i++;
                break;
            case RegexInstrOpCode_PushState:
                std::format_to(STDOUT, "PUSHSTATE");
                i++;
                break;
            case RegexInstrOpCode_PopState:
                std::format_to(STDOUT, "POPSTATE");
                i++;
                break;
            default:
                std::format_to(STDOUT, "UNKNOWN {}", static_cast<std::uint8_t>(bytecode[i]));
                i++;
                break;
        }

        std::format_to(STDOUT, "\n");
    }
}

class Regex
{
    struct RegexState
    {
        std::size_t program_counter;
        std::size_t string_position;
    };

    struct RegexVM
    {
        RegexByteCode& bytecode;
        std::size_t program_counter;
        std::size_t string_position;
        std::stack<RegexState> states;

        bool status_flag;
    };

    RegexByteCode _bytecode;

    bool _compile(const std::string& regex) noexcept
    {
        this->_bytecode.clear();

        std::format_to(STDOUT, "{}\n", regex);

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

        print_tokens(postfix_tokens);

        auto [emit_success, bytecode] = regex_emit(postfix_tokens);

        if(!emit_success)
        {
            std::format_to(STDERR, "Failed to emit bytecode for regex: {}\n", regex);
            return false;
        }

        regex_disasm(bytecode);

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