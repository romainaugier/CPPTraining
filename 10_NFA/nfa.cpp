#include <unordered_map>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <ranges>
#include <cstdint>
#include <stack>

using State = std::uint64_t;
using States = std::unordered_set<State>;
using Transition = std::pair<State, char>;
using Alphabet = std::unordered_set<char>;

struct TransitionHash 
{
    std::size_t operator()(const Transition& t) const 
    {
        return std::hash<State>{}(t.first) ^ (std::hash<char>{}(t.second) << 1);
    }
};

using Transitions = std::unordered_map<Transition, States, TransitionHash>;
using EpsilonTransitions = std::unordered_map<State, States>;

class NFA
{
    States _states;
    Alphabet _alphabet;
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
        Alphabet alphabet,
        Transitions transitions,
        EpsilonTransitions epsilon_transitions,
        States start_states,
        States accept_states) : _states(std::move(states)),
                                _alphabet(std::move(alphabet)),
                                _transitions(std::move(transitions)),
                                _epsilon_transitions(std::move(epsilon_transitions)),
                                _start_states(start_states),
                                _accept_states(std::move(accept_states)) {}

    bool accepts(const std::string& str) const noexcept
    {
        States current_states = this->_epsilon_closure(this->_start_states);

        for(const auto& s : str)
        {
            if(!this->_alphabet.contains(s))
            {
                return false;
            }

            States next_states;

            for(const auto& state : current_states)
            {
                Transition t(state, s);

                const auto transitions = this->_transitions.find(t);

                if(transitions != this->_transitions.end())
                {
                    next_states.insert(transitions->second.begin(), transitions->second.end());
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

int main(int argc, char** argv) 
{
    NFA nfa(States({ 0, 1, 2 }),
            Alphabet({ 'a', 'b' }),
            Transitions({ {{ 0, 'a' }, { 0, 1 }}, {{ 1, 'b' }, { 2 }}, {{ 2, 'a' }, { 2 }} }),
            EpsilonTransitions({ { 0, { 1 } } }),
            States({ 0 }),
            States({ 2 }));

    std::cout << "aab is accepted: " << nfa.accepts("aab") << "\n";   /* Should accept */
    std::cout << "aaba is accepted: " << nfa.accepts("aaba") << "\n"; /* Should accept */
    std::cout << "ab is accepted: " << nfa.accepts("ab") << "\n";     /* Should accept */
    std::cout << "b is accepted: " << nfa.accepts("b") << "\n";       /* Should accept */
    std::cout << "aabb is accepted: " << nfa.accepts("aabb") << "\n";
    std::cout << "aa is accepted: " << nfa.accepts("aa") << "\n";

    return 0;
}