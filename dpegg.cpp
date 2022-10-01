/*

Unusually detailed egg drop dynamic programming.

Fun way to "auto-discover" linear search and binary search and in-betweens, using brute-force.

The agent state consists of (E, lb, ub) where lb <= f* < ub.
E is the number of eggs in possession. Eggs that break are lost, eggs that do not break can be reused.
The special floor f* is the highest floor from which an egg-drop does not break.
The state is contrained such that 0 <= lb < ub.
The typical problem statement considers lb = 0, ub = F + 1, F = max floors.
Each drop executed requires a decision lb < f < ub; if ub = lb + 1, then f* = lb is localized already.
If the egg breaks, the upper bound is shifted.
If the egg does not break, the lower is shifted.

The optimal strategy minimizes the maximum number of drops required to find f*, given egg-budget E.
With sufficiently large E, the solution becomes binary search.
With E = 1, the solution is linear search from below.
The E = 2 case is more interesting.

The program computes two "data cubes", V and A, both indexed by (e, lb, ub).
V gives the worst case number of steps to go until the floor is localized.
A gives the action to take, i.e. the floor to drop from in the next attempt, for V to be true.

The program also generates all possible optimal egg-drop executions given initial state (E, 0, F + 1).
Then it is checked that the maximum number of drops really is given by V.
In addition, the mean number of egg-drops is found, and the histogram of possible outcomes.
The optimal strategy (for E > 1) has better worst case but also worse best case (compared to E = 1).
In general the actions are not entirely unique: same worst case, but different histograms.
The option --tiebreak is meant to produce a better histogram for the same optimal worst case. 
With --tiebreak, the mean number of drops across all possibilities should always be monotonic.
Otherwise only the worst case number of drops is monotonic (policy not unique).

BUILD:

  g++ -O2 -Wall -o dpegg dpegg.cpp
  clang++ -O2 -Wall -o dpegg dpegg.cpp

USAGE:
  ./dpegg F E [--tiebreak]

*/

// TODO: some sort of "minimizing interval" binary search is required to scale to large F
//       perhaps also store optimal drop range [f1, f2] for each decision node?

// NOTE: the degrees of freedom from average number of drops might be some "noisy" artifact..
//       basically there is an allowed interval where optimal drops are possible
//       (random selection in interval may be fine)

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <chrono>

int classic_dpegg_limit(int F, int E) {
  std::map<std::vector<int>, int> reach;  // reach[{d, e}] = max reachable with d drops and e eggs
  for (int e = 0; e <= E; e++)
    reach[{0, e}] = 0;
  for (int d = 1; d <= F; d++)
    reach[{d, 0}] = 0;
  for (int d = 1; d <= F; d++) {
    for (int e = 1; e <= E; e++) {
      const int N = 1 + reach[{d - 1, e - 1}] + reach[{d - 1, e}];
      if (N >= F)
        return d;
      reach[{d, e}] = N;
    }
  }
  return -1;
}

struct tState {

  bool operator==(const tState& rhs) const {
    return (eggs == rhs.eggs && lb == rhs.lb && ub == rhs.ub);
  }

  bool isterminal() const {
    return (ub == lb + 1 && lb >= 0 && eggs >= 0);
  }

  bool isfailed() const {
    return (eggs < 0 || (eggs == 0 && ub > lb + 1));
  }

  bool eggdrop(int floor, int limit) {
    const bool breaks = (floor > limit);
    if (breaks) {
      eggs--;
      if (floor < ub) ub = floor;
    } else {
      if (floor > lb) lb = floor;
    }
    return breaks;
  }

  tState next(int floor, int limit) const {
    tState state = {this->eggs, this->lb, this->ub};
    state.eggdrop(floor, limit);
    return state;    
  }

  int cost(int floor, int limit) const {
    return 1; // usual cost is 1 drop, independent of the floor dropped from
  }

  friend std::ostream& operator<<(std::ostream& os, const tState& s);

  int eggs;
  int lb;
  int ub; 
};

namespace std {
  template <>
  struct hash<tState> {
    size_t operator()(const tState& k) const {
      size_t res = 17;                             // http://stackoverflow.com/a/1646913/126995
      res = res * 31 + hash<int>()(k.eggs);
      res = res * 31 + hash<int>()(k.lb);
      res = res * 31 + hash<int>()(k.ub);
      return res;
    }
  };
}

std::ostream& operator<<(std::ostream& os, const tState& s) {
  os << "(e = " << s.eggs << ", lb = " << s.lb << ", ub = " << s.ub << ")";
  return os;
}

// Run optimal policy once and return number of drops required to localize the limit floor L
int run_policy_once(int F, 
                    int E, 
                    int L, 
                    const std::unordered_map<tState, int>& A,
                    std::vector<int>* aseq = nullptr)
{
  if (aseq != nullptr) aseq->clear();
  tState s = {E, 0, F + 1};
  int steps = 0;
  while (!s.isterminal()) {
    const auto s_search = A.find(s);
    const int a = s_search->second;
    s.eggdrop(a, L);
    steps++;
    if (aseq != nullptr) aseq->push_back(a);
  }
  return steps;
}

// Run the policy from the initial state {E, 0, F + 1}, for all possible limit floors 0..F
// Check that the worst case is indeed equal to the value stored in V, and also compute the mean number of drops.
// Optionally build histogram D across the floors, where the drops are done.
// Optionally build a histogram H of number of steps across all possible limit floors.
bool check_policy(int F, 
                  int E,
                  const std::unordered_map<tState, int>& V,
                  const std::unordered_map<tState, int>& A,
                  int& max_drops,
                  double& mean_drops,
                  std::vector<int>* D = nullptr,
                  std::unordered_map<int, int>* H = nullptr)
{
  if (D != nullptr) {
    D->clear();
    for (int l = 0; l <= F; l++)
      D->push_back(0);
  }
  if (H != nullptr)
    H->clear();
  std::vector<int> aseq;
  const tState s = {E, 0, F + 1};
  const auto s_search = V.find(s);
  const int nominal_value = s_search->second;
  int max_steps = 0;
  int sum_steps = 0;
  for (int l = 0; l <= F; l++) {  
    const int actual = run_policy_once(F, E, l, A, &aseq);
    if (H != nullptr)
      (*H)[actual]++;
    sum_steps += actual;
    if (actual > max_steps)
      max_steps = actual;
    if (D != nullptr)
      for (int action : aseq)
        (*D)[action]++;
  }
  max_drops = max_steps;
  mean_drops = static_cast<double>(sum_steps) / (F + 1); 
  return (max_drops == nominal_value);
}

int total_policy_at(const tState& snaught,
                    int action,
                    const std::unordered_map<tState, int>& A)
{
  int total_steps = 0;
  for (int l = snaught.lb; l < snaught.ub; l++) {
    tState s = snaught.next(action, l);
    int steps = 1;
    while (!s.isterminal()) {
      const auto s_search = A.find(s);
      const int a = s_search->second;
      s.eggdrop(a, l);
      steps++;
    }
    total_steps += steps;
  }
  return total_steps;
}

int argmin(const std::vector<int>& v) {
  std::vector<int>::const_iterator result = std::min_element(v.cbegin(), v.cend());
  return std::distance(v.cbegin(), result);
}

bool calc_maximum_value(const tState& s, 
                        int action,
                        const std::unordered_map<tState, int>& V,
                        int& max)
{
  int max_along_f = -1;
  bool all_ok = true;
  for (int f = s.lb; f < s.ub; f++) {
    tState nextState = s.next(action, f);
    auto search = V.find(nextState);
    bool this_ok = (search != V.end());
    all_ok = all_ok && this_ok;
    if (!all_ok) break;
    const int this_value = s.cost(action, f) + search->second;
    if (this_value > max_along_f)
      max_along_f = this_value;
  }
  max = max_along_f;
  return all_ok;
}

// An action is admissible if it can lead to a solution (i.e. not using all eggs inconclusively)
void find_admissible_actions(const tState& s, 
                             const std::unordered_map<tState, int>& V,
                             std::vector<int>& actions,
                             std::vector<int>& values,
                             bool break_on_increase)
{
  for (int a = s.lb + 1; a < s.ub; a++) {
    int themax = -1;
    const bool ok = calc_maximum_value(s, a, V, themax);
    if (!ok || themax == -1)
      continue;
    actions.push_back(a);
    values.push_back(themax);
    if (break_on_increase && actions.size() >= 2) {
      if (values[values.size() - 1] > values[values.size() - 2])
        break;
    }
  }
}

void print_all_admissible(const tState& s, 
                          const std::unordered_map<tState, int>& V,
                          const std::unordered_map<tState, int>* A = nullptr)
{
  std::vector<int> a;
  std::vector<int> v;
  const bool break_on_increase = false;
  find_admissible_actions(s, V, a, v, break_on_increase);
  std::cout << "--- decision @ state " << s << " ---" << std::endl;
  std::cout << "drops:  ";
  for (int a_ : a)
    std::cout << " " << a_;
  std::cout << std::endl;
  std::cout << "values: ";
  for (int v_ : v)
    std::cout << " " << v_;
  std::cout << std::endl;
  if (A == nullptr)
    return;
  std::cout << "means: ";
  for (int a_ : a) {       
    std::cout << " " << static_cast<double>(total_policy_at(s, a_, *A)) / (s.ub - s.lb);
  }
  std::cout << std::endl;
}

void initialize_terminal_entries(int F, 
                                 int E, 
                                 std::unordered_map<tState, int>& V) 
{
  for (int e = 0; e <= E; e++) {
    for (int f = 0; f <= F; f++) {
      V.insert({{e, f, f + 1}, 0});
    }
  }
}

void single_scan(int F, 
                 int Emin,
                 int Emax,
                 std::unordered_map<tState, int>& V,
                 std::unordered_map<tState, int>& A,
                 int& inserts,
                 int& modifies,
                 bool use_tiebreak = false,
                 int verbosity = 0)
{
  const bool break_early = true;

  std::vector<int> local_arg;
  std::vector<int> local_val;

  for (int e = Emin; e <= Emax; e++) {

    const int inserts_before_e = inserts;
    const int modifies_before_e = modifies;

    for (int l = F; l >= 0; l--) {
      for (int u = l + 1; u <= F + 1; u++) {
        tState thisState = {e, l, u};
        auto this_search = V.find(thisState);
        const bool thisExists = (this_search != V.end());
        if (thisExists && thisState.isterminal())
          continue;

        local_arg.clear();
        local_val.clear();

        find_admissible_actions(thisState, V, local_arg, local_val, break_early);

        if (local_val.size() == 0) {
          if (thisExists)
            std::cout << "existing nodes must have admissible actions" << std::endl;
          continue;
        }

        if (thisState.eggs == 1) {
          if (local_val.size() != 1)
            std::cout << "there should be exactly 1 admissible drop with 1 egg to-go" << std::endl;
        }

        if (!break_early) {
          if (thisState.eggs != 1 && static_cast<int>(local_val.size()) != thisState.ub - thisState.lb - 1) {
            std::cout << "unexpected no. of admissible drops: " << thisState << "; |A| = " << local_val.size() << std::endl; 
          }
        }
        
        int action_index = argmin(local_val);
        int action = local_arg[action_index];
        int value = local_val[action_index];

        if (verbosity > 1) {
          std::cout << "e,l,u=" << e << "," << l << "," << u << " allows: a=";
          for (auto a : local_arg)
            std::cout << a << " ";
          std::cout << std::endl << "val(a)=";
          for (auto va : local_val)
            std::cout << va << " ";
          std::cout << std::endl;
        }

        if (use_tiebreak) {
          std::vector<int> ties;
          std::vector<int> ties_totals;
          for (size_t i = 0; i < local_val.size(); i++) {
            if (local_val[i] == value) {
              ties.push_back(local_arg[i]);
              ties_totals.push_back(total_policy_at(thisState, local_arg[i], A));
            }
          }
          int sub_action_index = argmin(ties_totals);
          action = ties[sub_action_index];
        } else {
          std::vector<int> ties;
          for (size_t i = 0; i < local_val.size(); i++) {
            if (local_val[i] == value)
              ties.push_back(local_arg[i]);
          }
          action = ties[ties.size() >> 1];
        }

        if (thisExists) {
          auto this_search_a = A.find(thisState);
          if ((this_search->second > value) || 
              (this_search->second == value && this_search_a->second != action))  // use_tiebreak && 
          {
            this_search->second = value;
            this_search_a->second = action;
            modifies++;
          }
        }
        else {
          V.insert({thisState, value});
          A.insert({thisState, action});
          inserts++;
        }

      }
    }

    const int total_edits_at_e = (inserts - inserts_before_e) + (modifies - modifies_before_e);
    if (verbosity > 0)
      std::cout << "level e = " << e << " had " << total_edits_at_e << " value edits" << std::endl;
  }
}

std::string histogram_to_string(const std::unordered_map<int, int>& H, int kmin, int kmax) {
  std::string s = "";
  for (int k = kmin; k <= kmax; k++) {
    const auto search = H.find(k);
    if (search == H.end())
      s += "0 ";
    else
      s += std::to_string(search->second) + " ";
  }
  return s;
}

int as_integer(const char* str) {
  return static_cast<int>(std::strtol(str, nullptr, 0));
}

/*****************************************************************************/

int main(int argc, char** argv)
{
  if (argc != 3 && argc != 4) {
    std::cout << "usage: " << argv[0] << " F E [--tiebreak]" << std::endl;
    return 1;
  }

  const int F = as_integer(argv[1]);
  const int E = as_integer(argv[2]);

  if (F <= 0 || E <= 0) {
    std::cout << "invalid input(s): F, E >= 1 required" << std::endl;
    return 1;
  }

  const bool use_tiebreak = (argc == 4 && std::string(argv[3]) == "--tiebreak");

  if (argc == 4 && !use_tiebreak) {
    std::cout << "invalid input(s): only option --tiebreak is recognized" << std::endl;
    return 1;
  }

  // parrot this call for later reference
  for (int i = 0; i < argc; i++)
    std::cout << argv[i] << " ";
  std::cout << std::endl;

  std::cout << "--- required min. number of drops = " << classic_dpegg_limit(F, E) << std::endl;

  std::unordered_map<tState, int> V; // "value function"
  std::unordered_map<tState, int> A; // "control action"

  auto clock_start = std::chrono::high_resolution_clock::now();

  initialize_terminal_entries(F, E, V);
 
  for (int e = 1; e <= E; e++) {
    for (int s = 0; ; s++) {
      int inserts = 0;
      int modifies = 0;
      single_scan(F, e, e, V, A, inserts, modifies, use_tiebreak);
      //std::cout << "[e = " << e << ", scan = " << s << "]: " << "inserts = " << inserts << ", modifies = " << modifies << std::endl;
      if (inserts == 0 && modifies == 0) {
        std::cout << s + 1 << " scans at level e = " << e << std::endl;
        break;
      }
    }
  }

  auto clock_end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> clock_diff = clock_end - clock_start;

  std::cout << std::setprecision(6);

  std::cout << "value (action) table has " << V.size() << " (" << A.size() 
            << ") entries (duration = " << clock_diff.count() << " s.)" << std::endl;

  std::unordered_map<int, int> histo;
  std::vector<std::vector<int>> drops;
  int max_drops;
  double mean_drops;

  drops.emplace_back();

  for (int e = 1; e <= E; e++) {

    drops.emplace_back();

    const bool looks_ok = check_policy(F, e, V, A, max_drops, mean_drops, &drops[e], &histo);

    if (!looks_ok) {
      std::cout << "DP solution is inconsistent (e = " << e << ")" << std::endl;
      return 1;
    }

    std::cout << "--- floors F = " << F << ", eggs E = " << e << " ---" << std::endl;
    std::cout << "min max drops = " << max_drops << " (optimal worst case)" << std::endl;
    std::cout << "mean drops    = " << mean_drops << " (uniform limit floor)" << std::endl;
    std::cout << "drops histg.  = " << histogram_to_string(histo, 0, max_drops) << std::endl;

    print_all_admissible({e, 0, F + 1}, V, &A);
  }

  std::cout << "--- min max drops, E = 1.." << E << " ---" << std::endl;
  for (int f = 1; f <= F; f++) {
    std::cout << "floors " << std::setw(3) << f << ": ";
    for (int e = 1; e <= E; e++) {
      auto itr = V.find({e, 0, f + 1});
      std::cout << std::setw(3) << itr->second << " ";
    }
    std::cout << std::endl;
  }

  // this table may not be monotonic in general (along F) unless --tiebreak is specified!
  std::cout << "--- average drops, E = 1.." << E << " ---" << std::endl;
  for (int f = 1; f <= F; f++) {
    std::cout << "floors " << std::setw(3) << f << ": ";
    for (int e = 1; e <= E; e++) {
      check_policy(f, e, V, A, max_drops, mean_drops);
      std::cout << std::setw(8) << mean_drops << " ";
    }
    std::cout << std::endl;
  }

  std::cout << "--- drop histograms E = 1.." << E << " (F = " << F << ") ---" << std::endl;
  for (int f = 1; f <= F; f++) {
    std::cout << "floor  " << std::setw(3) << f << ": ";
    for (int e = 1; e <= E; e++)
      std::cout << std::setw(3) << drops[e][f] << " ";
    std::cout << std::endl;
  }

  std::cout << "--- optimal E = " << E << " executions for all limit levels L ---" << std::endl;
  for (int x = 0; x <= F; x++) {
    std::vector<int> aseq;
    int xsteps = run_policy_once(F, E, x, A, &aseq);
    std::cout << "L = " << std::setw(3) << x << ": ";
    for (int y : aseq)
      std::cout << y << " ";
    std::cout << "(" << xsteps << " steps)" << std::endl;
  }

  return 0;
}
