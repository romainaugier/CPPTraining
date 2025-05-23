#include <unordered_map>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <ranges>

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

class NFA
{
    States _states;
    Alphabet _alphabet;
    Transitions _transitions;

    State _start_state;
    States _accept_states;

public:
    NFA(States states,
        Alphabet alphabet,
        Transitions transitions,
        State start_state,
        States accept_states) : _states(std::move(states)),
                                _alphabet(std::move(alphabet)),
                                _transitions(std::move(transitions)),
                                _start_state(start_state),
                                _accept_states(std::move(accept_states)) {}

    bool accepts(const std::string& str) const noexcept
    {
        States current_states({ this->_start_state });

        for(const auto& s : str)
        {
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

            current_states = std::move(next_states);

            if(current_states.empty())
            {
                return false;
            }
        }

        /*
        std::vector<State> intersection;

        std::set_intersection(current_states.begin(), current_states.end(), this->_accept_states.begin(), this->_accept_states.end(), std::back_inserter(intersection));

        return intersection.size() > 0;
        */

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
            0,
            States({ 2 }));

    std::cout << "aab is accepted: " << nfa.accepts("aab") << "\n";
    std::cout << "ab is accepted: " << nfa.accepts("ab") << "\n";
    std::cout << "aabb is accepted: " << nfa.accepts("aabb") << "\n";
    std::cout << "aa is accepted: " << nfa.accepts("aa") << "\n";

    return 0;
}