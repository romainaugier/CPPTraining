#include <vector>
#include <string>
#include <iostream>
#include <cstdint>
#include <stack>
#include <format>
#include <cctype>
#include <tuple>
#include <cassert>

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
        if(std::isalnum(regex[i]) || regex[i] == '_')
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
inline int get_operator_precedence(RegexOperatorType op) noexcept
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

inline bool is_left_associative(RegexOperatorType op) noexcept
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

    std::uint32_t current_group_id = 1;

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
                      operators.top().type != RegexTokenType_GroupBegin)
                { 
                    if(operators.top().type != RegexTokenType_Operator)
                    {
                        break;
                    }

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
                RegexToken group_token = token;
                group_token.encoding = current_group_id++;
                operators.push(group_token);

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

                RegexToken group_begin_token = operators.top();
                operators.pop();

                // RegexToken group_end_token = token;
                // group_end_token.encoding = --current_group_id;

                // output.push_back(group_end_token);

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
    for(std::size_t i = 0; i < tokens.size(); ++i)
    {
        tokens[i].print();
        std::format_to(STDOUT, "\n");
    }
}

/* Instructions / Bytecode */

using RegexByteCode = std::vector<std::byte>;

enum RegexInstrOpCode : std::uint8_t
{
    RegexInstrOpCode_TestSingle,
    RegexInstrOpCode_TestRange,
    RegexInstrOpCode_TestNegatedRange,
    RegexInstrOpCode_TestAny,
    RegexInstrOpCode_TestDigit,
    RegexInstrOpCode_TestLowerCase,
    RegexInstrOpCode_TestUpperCase,
    RegexInstrOpCode_JumpEq, /* jumps if status_flag = true */
    RegexInstrOpCode_JumpNeq, /* jumps if status_flag = false */
    RegexInstrOpCode_Accept,
    RegexInstrOpCode_Fail,
    RegexInstrOpCode_GroupStart,
    RegexInstrOpCode_GroupEnd,
    RegexInstrOpCode_IncPos,     /* ++string_pos */
    RegexInstrOpCode_DecPos,     /* --string_pos */
    RegexInstrOpCode_IncPosEq, /* string_pos += (status_flag ? 1 : 0) */
    RegexInstrOpCode_JumpPos,    /* Absolute jump in string position */
    RegexInstrOpCode_SetFlag, /* status_flag = static_cast<bool>(bytecode[pc + 1]) */
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

inline RegexByteCode get_range_instrs(char range_start, char range_end) noexcept
{
    if(range_start == '0' && range_end == '9')
    {
        return { BYTE(RegexInstrOpCode_TestDigit), BYTE(RegexInstrOpCode_IncPosEq) };
    }
    else if(range_start == 'a' && range_end == 'z')
    {
        return { BYTE(RegexInstrOpCode_TestLowerCase), BYTE(RegexInstrOpCode_IncPosEq) };
    }
    else if(range_start == 'A' && range_end == 'Z')
    {
        return { BYTE(RegexInstrOpCode_TestUpperCase), BYTE(RegexInstrOpCode_IncPosEq) };
    }
    else
    {
        return { 
                    BYTE(RegexInstrOpCode_TestRange),
                    BYTE(range_start),
                    BYTE(range_end),
                    BYTE(RegexInstrOpCode_IncPosEq)
                };
    }
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
            {
                RegexByteCode frag;

                switch(token.encoding)
                {
                    case RegexCharacterType_Single:
                        frag.push_back(BYTE(RegexInstrOpCode_TestSingle));
                        frag.push_back(BYTE(token.data[0]));
                        break;

                    case RegexCharacterType_Any:
                        frag.push_back(BYTE(RegexInstrOpCode_TestAny));
                        break;
                }
                
                frag.push_back(BYTE(RegexInstrOpCode_IncPosEq));

                fragments.push({ RegexInstrOpType_TestOp, std::move(frag) });

                break;
            }

            case RegexTokenType_CharacterRange:
                fragments.push({ 
                    RegexInstrOpType_TestOp,
                    get_range_instrs(token.data[0], token.data[2])
                });
                break;

            case RegexTokenType_GroupBegin:
                fragments.push({ 
                    RegexInstrOpType_GroupOp, 
                    { 
                        BYTE(RegexInstrOpCode_GroupStart),
                        BYTE(token.encoding & 0xFF)
                    } 
                });
                break;

            case RegexTokenType_GroupEnd:
                fragments.push({
                    RegexInstrOpType_GroupOp, 
                    { 
                        BYTE(RegexInstrOpCode_GroupEnd),
                        BYTE(token.encoding & 0xFF)
                    } 
                });
                break;

            case RegexTokenType_Operator:
            {
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

                        const std::size_t jump_over_rhs_pos = emit_jump(frag, RegexInstrOpCode_JumpEq);

                        frag.insert(frag.end(), rhs.begin(), rhs.end());

                        const int offset = static_cast<int>(frag.size() - (jump_over_rhs_pos - 1));
                        patch_jump(frag, jump_over_rhs_pos, offset);

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

                        const std::size_t jump_pos = emit_jump(frag, RegexInstrOpCode_JumpNeq);
                        patch_jump(frag, jump_pos, JUMP_FAIL);

                        frag.insert(frag.end(), rhs.begin(), rhs.end());

                        fragments.push({ RegexInstrOpType_BinaryOp, std::move(frag) });

                        break;
                    }

                    case RegexOperatorType_ZeroOrMore:
                    {
                        RegexByteCode frag; 

                        const auto [op_type, body] = std::move(fragments.top());
                        fragments.pop();

                        const std::size_t loop_body_start = frag.size();
                        frag.insert(frag.end(), body.begin(), body.end());
                        
                        const int offset_back = static_cast<int>(loop_body_start - (frag.size()));
                        const std::size_t loop_jump_pos = emit_jump(frag, RegexInstrOpCode_JumpEq);
                        patch_jump(frag, loop_jump_pos, offset_back);
                        frag.push_back(BYTE(RegexInstrOpCode_SetFlag));
                        frag.push_back(BYTE(1));
                        
                        fragments.push({ RegexInstrOpType_UnaryOp, std::move(frag) });

                        break;
                    }

                    case RegexOperatorType_OneOrMore:
                    {
                        RegexByteCode frag; 

                        const auto [op_type, body] = std::move(fragments.top());
                        fragments.pop();

                        const std::size_t initial_match_start = frag.size();
                        frag.insert(frag.end(), body.begin(), body.end());
                        
                        const std::size_t fail_jump_pos = emit_jump(frag, RegexInstrOpCode_JumpNeq);
                        patch_jump(frag, fail_jump_pos, JUMP_FAIL);

                        const std::size_t loop_start = frag.size();
                        frag.insert(frag.end(), body.begin(), body.end());
                        
                        const std::size_t exit_loop_jump_pos = emit_jump(frag, RegexInstrOpCode_JumpNeq);
                        frag.push_back(BYTE(RegexInstrOpCode_SetFlag));
                        frag.push_back(BYTE(1));
                        
                        const int offset_back = static_cast<int>(loop_start - frag.size());
                        const std::size_t loop_jump_pos = emit_jump(frag, RegexInstrOpCode_JumpEq);
                        patch_jump(frag, loop_jump_pos, offset_back);
                        
                        patch_jump(frag, exit_loop_jump_pos, static_cast<int>(frag.size() - (exit_loop_jump_pos-1)));
                        
                        fragments.push({ RegexInstrOpType_UnaryOp, std::move(frag) });

                        break;
                    }

                    case RegexOperatorType_ZeroOrOne:
                    {
                        RegexByteCode frag; 

                        auto [op_type, body] = std::move(fragments.top());
                        fragments.pop();
                        
                        frag.insert(frag.end(), body.begin(), body.end());
                        frag.push_back(BYTE(RegexInstrOpCode_SetFlag));
                        frag.push_back(BYTE(1));
                        
                        fragments.push({ RegexInstrOpType_UnaryOp, std::move(frag) });

                        break;
                    }
                }

                break;
            }
        }
    }

    if(fragments.empty()) 
    {
        RegexByteCode bytecode;
        bytecode.push_back(BYTE(RegexInstrOpCode_Accept));
        return { true, std::move(bytecode) };
    }

    auto [last_op_type, bytecode] = std::move(fragments.top());
    fragments.pop();

    const std::size_t final_fail_jump = emit_jump(bytecode, RegexInstrOpCode_JumpNeq);
    patch_jump(bytecode, final_fail_jump, JUMP_FAIL);
    
    bytecode.push_back(BYTE(RegexInstrOpCode_Accept));

    const std::size_t fail_addr = bytecode.size();
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
                    offset = static_cast<int>(bytecode.size() - (i + 5));

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
            case RegexInstrOpCode_TestAny:
            case RegexInstrOpCode_TestDigit:
            case RegexInstrOpCode_TestLowerCase:
            case RegexInstrOpCode_TestUpperCase:
            case RegexInstrOpCode_IncPos:
            case RegexInstrOpCode_DecPos:
            case RegexInstrOpCode_IncPosEq:
                i++;
                break;

            case RegexInstrOpCode_JumpPos:
                i += 5;
                break;

            case RegexInstrOpCode_TestSingle:
            case RegexInstrOpCode_SetFlag:
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

std::tuple<bool, RegexByteCode> regex_opt(RegexByteCode& bytecode) noexcept
{
    std::size_t i = 0;

    while(i < bytecode.size())
    {

    }

    return std::make_tuple(true, bytecode);
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
            case RegexInstrOpCode_TestDigit:
                std::format_to(STDOUT, "TESTDIGIT");
                i++;
                break;
            case RegexInstrOpCode_TestLowerCase:
                std::format_to(STDOUT, "TESTLOWERCASE");
                i++;
                break;
            case RegexInstrOpCode_TestUpperCase:
                std::format_to(STDOUT, "TESTUPPERCASE");
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
                std::format_to(STDOUT, "GROUPSTART {}", static_cast<std::uint8_t>(bytecode[i + 1]));
                i += 2;
                break;
            case RegexInstrOpCode_GroupEnd:
                std::format_to(STDOUT, "GROUPEND {}", static_cast<std::uint8_t>(bytecode[i + 1]));
                i += 2;
                break;
            case RegexInstrOpCode_IncPos:
                std::format_to(STDOUT, "INCPOS");
                i++;
                break;
            case RegexInstrOpCode_DecPos:
                std::format_to(STDOUT, "DECPOS");
                i++;
                break;
            case RegexInstrOpCode_IncPosEq:
                std::format_to(STDOUT, "INCPOSEQ");
                i++;
                break;
            case RegexInstrOpCode_JumpPos:
                std::format_to(STDOUT,
                               "JUMPPOS {}{}",
                               decode_jump(std::addressof(bytecode[i + 1])) >= 0 ? "+" : "",
                               decode_jump(std::addressof(bytecode[i + 1])));
                i += 5;
                break;
            case RegexInstrOpCode_SetFlag:
                std::format_to(STDOUT,
                               "SETFLAG {}",
                               static_cast<std::uint8_t>(bytecode[i + 1]));
                i += 2;
                break;
            default:
                std::format_to(STDOUT, "UNKNOWN {}", static_cast<std::uint8_t>(bytecode[i]));
                i++;
                break;
        }

        std::format_to(STDOUT, "\n");
    }
}

struct RegexVM
{
    const RegexByteCode& bytecode;

    std::string string;
    std::size_t sp; /* string position */
    std::size_t pc; /* program counter */

    bool status_flag;

    RegexVM(const RegexByteCode& bytecode, const std::string& str) : bytecode(bytecode), 
                                                                     string(str),
                                                                     sp(0),
                                                                     pc(0),
                                                                     status_flag(false) {}

    RegexVM(const RegexVM&) = delete;
    RegexVM& operator=(const RegexVM&) = delete;

    RegexVM(RegexVM&&) = delete;
    RegexVM& operator=(RegexVM&&) = delete;
};

bool regex_exec(RegexVM* vm) noexcept
{
    while(vm->pc < vm->bytecode.size() && vm->sp <= vm->string.size())
    {
        switch(static_cast<std::uint8_t>(vm->bytecode[vm->pc]))
        {
            case RegexInstrOpCode_TestSingle:
                vm->status_flag = vm->string[vm->sp] == CHAR(vm->bytecode[vm->pc + 1]);
                vm->pc += 2;
                break;
            case RegexInstrOpCode_TestRange:
                vm->status_flag = vm->string[vm->sp] >= CHAR(vm->bytecode[vm->pc + 1]) &&
                                  vm->string[vm->sp] <= CHAR(vm->bytecode[vm->pc + 2]);
                vm->pc += 3;
                break;
            case RegexInstrOpCode_TestNegatedRange:
                vm->status_flag = vm->string[vm->sp] < CHAR(vm->bytecode[vm->pc + 1]) &&
                                  vm->string[vm->sp] > CHAR(vm->bytecode[vm->pc + 2]);
                vm->pc += 3;
                break;
            case RegexInstrOpCode_TestAny:
                vm->status_flag = true;
                vm->pc += 1;
                break;
            case RegexInstrOpCode_TestDigit:
                vm->status_flag = std::isdigit(static_cast<int>(vm->string[vm->sp]));
                vm->pc += 1;
                break;
            case RegexInstrOpCode_TestLowerCase:
                vm->status_flag = std::islower(static_cast<int>(vm->string[vm->sp]));
                vm->pc += 1;
                break;
            case RegexInstrOpCode_TestUpperCase:
                vm->status_flag = std::isupper(static_cast<int>(vm->string[vm->sp]));
                vm->pc += 1;
                break;
            case RegexInstrOpCode_JumpEq:
                if(vm->status_flag)
                {
                    const int jmp = decode_jump(std::addressof(vm->bytecode[vm->pc + 1]));

                    if(jmp > 0)
                    {
                        vm->pc += 5;
                    }
                    
                    vm->pc += jmp;
                }
                else
                {
                    vm->pc += 5;
                }

                break;
            case RegexInstrOpCode_JumpNeq:
                if(!vm->status_flag)
                {
                    const int jmp = decode_jump(std::addressof(vm->bytecode[vm->pc + 1]));

                    if(jmp > 0)
                    {
                        vm->pc += 5;
                    }
                    
                    vm->pc += jmp;
                }
                else
                {
                    vm->pc += 5;
                }
                break;
            case RegexInstrOpCode_Accept:
                return true;
            case RegexInstrOpCode_Fail:
                return false;
            case RegexInstrOpCode_GroupStart:
                vm->pc += 2;
                break;
            case RegexInstrOpCode_GroupEnd:
                vm->pc += 2;
                break;
            case RegexInstrOpCode_IncPos:
                vm->sp++;
                vm->pc++;
                break;
            case RegexInstrOpCode_DecPos:
                vm->sp--;
                vm->pc++;
                break;
            case RegexInstrOpCode_IncPosEq:
                vm->sp += static_cast<std::size_t>(vm->status_flag);
                vm->pc++;
                break;
            case RegexInstrOpCode_JumpPos:
                vm->sp += decode_jump(std::addressof(vm->bytecode[vm->pc + 1]));
                vm->pc += 5;
                break;
            case RegexInstrOpCode_SetFlag:
                vm->status_flag = static_cast<bool>(vm->bytecode[vm->pc + 1]);
                vm->pc += 2;
                break;
            default:
                std::format_to(STDERR,
                               "Error: unknown instruction in regex vm: {} (pc {})",
                               static_cast<std::uint8_t>(vm->bytecode[vm->pc]),
                               vm->pc);
                return false;
        }
    }

    return false;
}

class Regex
{
    RegexByteCode _bytecode;

    bool _compile(const std::string& regex, const bool debug) noexcept
    {
        this->_bytecode.clear();

        if(debug)
        {
            std::format_to(STDOUT, "Compiling Regex: {}\n", regex);
        }

        auto [lex_success, tokens] = lex_regex(regex);

        if(!lex_success)
        {
            std::format_to(STDERR, "Failed to lex regex: {}\n", regex);
            return false;
        }

        if(debug)
        {
            std::format_to(STDOUT, "Regex tokens (lex):\n");
            print_tokens(tokens);
        }

        auto [parse_success, postfix_tokens] = parse_regex(tokens);

        if(!parse_success)
        {
            std::format_to(STDERR, "Failed to parse regex: {}\n", regex);
            return false;
        }

        if(debug)
        {
            std::format_to(STDOUT, "Regex tokens (parse):\n");
            print_tokens(tokens);
        }

        auto [emit_success, bytecode] = regex_emit(postfix_tokens);

        if(!emit_success)
        {
            std::format_to(STDERR, "Failed to emit bytecode for regex: {}\n", regex);
            return false;
        }

        this->_bytecode = std::move(bytecode);

        if(debug)
        {
            std::format_to(STDOUT, "Regex disasm:\n");
            regex_disasm(this->_bytecode);
        }

        return true;
    }

public:
    Regex(const std::string& regex, const bool debug_compilation = false)
    {
        this->_compile(regex, debug_compilation);
    }

    bool match(const std::string& str) const noexcept
    {
        if(this->_bytecode.size() == 0)
        {
            return false;
        }

        RegexVM vm(this->_bytecode, str);

        bool res = regex_exec(&vm);

        return res;
    }
};

// int main(int argc, char** argv) 
// {
//     Regex regex1("[0-9]*", true);

//     assert(regex1.match("123456789"));
//     assert(regex1.match("12345abcde"));
//     assert(regex1.match("abcde12345"));

//     Regex regex2("[0-9]+", true);

//     assert(regex2.match("123456789"));
//     assert(regex2.match("12345abcde"));
//     assert(!regex2.match("abcde12345"));

//     Regex regex3("a*b|cd");

//     assert(regex3.match("aaaaaacd"));
//     assert(regex3.match("abd"));
//     assert(regex3.match("bd"));
//     assert(regex3.match("cd"));
//     assert(!regex3.match("aaaacacd"));

//     Regex regex4("a?([b-e])+");

//     assert(regex4.match("abcdebcde"));
//     assert(regex4.match("bcdebcde"));
//     assert(!regex4.match("rbcdebcde"));

//     return 0;
// }