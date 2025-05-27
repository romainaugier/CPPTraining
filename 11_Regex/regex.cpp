#include <unordered_map>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <ranges>
#include <cstdint>
#include <stack>
#include <functional>

using MatchFunc = std::function<bool(char)>;
using State = std::uint64_t;
using States = std::unordered_set<State>;

struct Transition 
{
    States to;

    MatchFunc match;
};

using Transitions = std::unordered_map<State, std::vector<Transition>>;
using EpsilonTransitions = std::unordered_map<State, States>;

class NFA
{
    States _states;
    Transitions _transitions;
    EpsilonTransitions _epsilon_transitions;

    States _start_states;
    States _accept_states;

    States _epsilon_closure(const States& states) const noexcept
    {
        States new_states = states;

        std::stack<State> stack;

        for(const auto& state : states)
        {
            stack.push(state);
        }

        while(!stack.empty())
        {
            State state = stack.top();
            stack.pop();

            auto it = this->_epsilon_transitions.find(state);

            if(it != this->_epsilon_transitions.end())
            {
                for(const auto& next : it->second)
                {
                    if(new_states.insert(next).second)
                    {
                        stack.push(next);
                    }
                }
            }
        }

        return new_states;
    }

public:
    NFA(States states,
        Transitions transitions,
        EpsilonTransitions epsilon_transitions,
        States start_states,
        States accept_states) : _states(std::move(states)),
                                _transitions(std::move(transitions)),
                                _epsilon_transitions(std::move(epsilon_transitions)),
                                _start_states(start_states),
                                _accept_states(std::move(accept_states)) {}

    bool accepts(const std::string& str) const noexcept
    {
        States current_states = this->_epsilon_closure(this->_start_states);

        for(const auto& s : str)
        {
            States next_states;

            for(const auto& state : current_states)
            {
                const auto transitions = this->_transitions.find(state);

                if(transitions != this->_transitions.end())
                {
                    for(const auto& transition : transitions->second)
                    {
                        if(transition.match(s))
                        {
                            next_states.insert(transition.to.begin(), transition.to.end());
                        }
                    }
                }
            }

            current_states = this->_epsilon_closure(next_states);

            if(current_states.empty())
            {
                return false;
            }
        }

        return std::ranges::any_of(current_states, [this](State s) -> bool {
            return this->_accept_states.contains(s);
        });
    }
};

NFA nfa_from_regex(const std::string& regex) noexcept
{
    States states;

    for(const char c : regex)
    {
        State current_state = states.size();
    }
}

int main(int argc, char** argv) 
{
    NFA nfa = nfa_from_regex("(a|b)*");

    return 0;
}