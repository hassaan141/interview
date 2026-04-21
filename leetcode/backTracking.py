"""
BACKTRACKING ALGORITHM EXPLAINED
===============================

Backtracking is a general algorithmic approach that considers searching every possible combination
in order to solve computational problems. It incrementally builds candidates to the solutions,
and abandons candidates ("backtracks") as soon as it determines that they cannot possibly be 
extended to a valid solution.

GENERAL TEMPLATE:
================
def backtrack(candidate):
    if find_solution(candidate):
        output(candidate)
        return
    
    # Iterate all possible candidates
    for next_candidate in list_of_candidates:
        if is_valid(next_candidate):
            # Make a choice
            place(next_candidate)
            
            # Explore further with this choice
            backtrack(next_candidate)
            
            # Backtrack - undo the choice
            remove(next_candidate)

KEY CONCEPTS:
============
1. Choice: What choices do we have at each step?
2. Constraints: When is a choice invalid?
3. Goal: When have we reached a solution?
4. Backtrack: How do we undo a choice?
"""

# =============================================================================
# EXAMPLE 1: GENERATE ALL PERMUTATIONS
# =============================================================================

def generate_permutations(nums):
    """
    Generate all possible permutations of a list of numbers.
    
    Time Complexity: O(N! * N) where N is the length of nums
    Space Complexity: O(N) for recursion stack
    """
    result = []
    
    def backtrack(current_permutation):
        # Base case: if current permutation is complete
        if len(current_permutation) == len(nums):
            result.append(current_permutation[:])  # Make a copy
            return
        
        # Try each number that hasn't been used yet
        for num in nums:
            if num in current_permutation:
                continue  # Skip if already used
            
            # Make choice
            current_permutation.append(num)
            
            # Recurse
            backtrack(current_permutation)
            
            # Backtrack
            current_permutation.pop()
    
    backtrack([])
    return result


# =============================================================================
# EXAMPLE 2: GENERATE ALL SUBSETS (POWER SET)
# =============================================================================

# def generate_subsets(nums):
#     """
#     Generate all possible subsets of a list of numbers.
    
#     Time Complexity: O(2^N * N) where N is the length of nums
#     Space Complexity: O(N) for recursion stack
#     """
#     result = []
    
#     def backtrack(start, current_subset):
#         # Add current subset to result (every subset is valid)
#         result.append(current_subset[:])  # Make a copy
        
#         # Try adding each remaining number
#         for i in range(start, len(nums)):
#             # Make choice
#             current_subset.append(nums[i])
            
#             # Recurse with next index
#             backtrack(i + 1, current_subset)
            
#             # Backtrack
#             current_subset.pop()
    
#     backtrack(0, [])
#     return result

def generate_subsets(nums):
    """
    Generate all possible subsets of a list of numbers.
    
    Time Complexity: O(2^N * N) where N is the length of nums
    Space Complexity: O(N) for recursion stack
    """
    
    result = []
    
    return result


# =============================================================================
# EXAMPLE 3: N-QUEENS PROBLEM
# =============================================================================

def solve_n_queens(n):
    """
    Place N queens on an N×N chessboard so that no two queens attack each other.
    
    Time Complexity: O(N!)
    Space Complexity: O(N^2)
    """
    result = []
    board = [['.'] * n for _ in range(n)]
    
    def is_safe(row, col):
        # Check column
        for i in range(row):
            if board[i][col] == 'Q':
                return False
        
        # Check diagonal (top-left to bottom-right)
        for i, j in zip(range(row-1, -1, -1), range(col-1, -1, -1)):
            if board[i][j] == 'Q':
                return False
        
        # Check diagonal (top-right to bottom-left)
        for i, j in zip(range(row-1, -1, -1), range(col+1, n)):
            if board[i][j] == 'Q':
                return False
        
        return True
    
    def backtrack(row):
        if row == n:
            # Found a solution
            result.append([''.join(row) for row in board])
            return
        
        for col in range(n):
            if is_safe(row, col):
                # Make choice
                board[row][col] = 'Q'
                
                # Recurse
                backtrack(row + 1)
                
                # Backtrack
                board[row][col] = '.'
    
    backtrack(0)
    return result


# =============================================================================
# EXAMPLE 4: SUDOKU SOLVER
# =============================================================================

def solve_sudoku(board):
    """
    Solve a 9x9 Sudoku puzzle using backtracking.
    
    Time Complexity: O(9^(N*N)) in worst case
    Space Complexity: O(N*N) for recursion stack
    """
    def is_valid(board, row, col, num):
        # Check row
        for j in range(9):
            if board[row][j] == num:
                return False
        
        # Check column
        for i in range(9):
            if board[i][col] == num:
                return False
        
        # Check 3x3 box
        start_row = (row // 3) * 3
        start_col = (col // 3) * 3
        for i in range(start_row, start_row + 3):
            for j in range(start_col, start_col + 3):
                if board[i][j] == num:
                    return False
        
        return True
    
    def backtrack():
        for i in range(9):
            for j in range(9):
                if board[i][j] == '.':
                    for num in '123456789':
                        if is_valid(board, i, j, num):
                            # Make choice
                            board[i][j] = num
                            
                            # Recurse
                            if backtrack():
                                return True
                            
                            # Backtrack
                            board[i][j] = '.'
                    
                    return False  # No valid number found
        
        return True  # All cells filled
    
    backtrack()
    return board


# =============================================================================
# EXAMPLE 5: WORD SEARCH IN GRID
# =============================================================================

def word_search(board, word):
    """
    Find if a word exists in a 2D grid of characters.
    
    Time Complexity: O(N * M * 4^L) where N, M are grid dimensions and L is word length
    Space Complexity: O(L) for recursion stack
    """
    rows, cols = len(board), len(board[0])
    directions = [(0, 1), (1, 0), (0, -1), (-1, 0)]  # right, down, left, up
    
    def backtrack(row, col, index):
        # Base case: found the word
        if index == len(word):
            return True
        
        # Check bounds and character match
        if (row < 0 or row >= rows or 
            col < 0 or col >= cols or 
            board[row][col] != word[index] or 
            board[row][col] == '#'):  # '#' means visited
            return False
        
        # Mark as visited
        temp = board[row][col]
        board[row][col] = '#'
        
        # Explore all directions
        found = False
        for dr, dc in directions:
            if backtrack(row + dr, col + dc, index + 1):
                found = True
                break
        
        # Backtrack: restore original character
        board[row][col] = temp
        
        return found
    
    # Try starting from every cell
    for i in range(rows):
        for j in range(cols):
            if backtrack(i, j, 0):
                return True
    
    return False


# =============================================================================
# EXAMPLE 6: COMBINATION SUM
# =============================================================================

def combination_sum(candidates, target):
    """
    Find all unique combinations where candidate numbers sum to target.
    Numbers can be used multiple times.
    
    Time Complexity: O(N^(T/M)) where N is array length, T is target, M is minimal value
    Space Complexity: O(T/M) for recursion stack
    """
    result = []
    
    def backtrack(start, current_combination, remaining_sum):
        # Base case: found a valid combination
        if remaining_sum == 0:
            result.append(current_combination[:])  # Make a copy
            return
        
        # Base case: sum exceeded
        if remaining_sum < 0:
            return
        
        for i in range(start, len(candidates)):
            # Make choice
            current_combination.append(candidates[i])
            
            # Recurse (can use same number again, so pass i, not i+1)
            backtrack(i, current_combination, remaining_sum - candidates[i])
            
            # Backtrack
            current_combination.pop()
    
    backtrack(0, [], target)
    return result


# =============================================================================
# EXAMPLE 7: GENERATE PARENTHESES
# =============================================================================

def generate_parentheses(n):
    """
    Generate all combinations of well-formed parentheses for n pairs.
    
    Time Complexity: O(4^n / √n) - Catalan number
    Space Complexity: O(n) for recursion stack
    """
    result = []
    
    def backtrack(current, open_count, close_count):
        # Base case: used all n pairs
        if len(current) == 2 * n:
            result.append(current)
            return
        
        # Add opening parenthesis if we haven't used all
        if open_count < n:
            backtrack(current + '(', open_count + 1, close_count)
        
        # Add closing parenthesis if it won't exceed opening ones
        if close_count < open_count:
            backtrack(current + ')', open_count, close_count + 1)
    
    backtrack('', 0, 0)
    return result


# =============================================================================
# TESTING THE EXAMPLES
# =============================================================================

if __name__ == "__main__":
    # Test permutations
    print("=== PERMUTATIONS ===")
    print("Permutations of [1,2,3]:")
    perms = generate_permutations([1, 2, 3])
    for perm in perms:
        print(perm)
    
    print("\n=== SUBSETS ===")
    print("Subsets of [1,2,3]:")
    subsets = generate_subsets([1, 2, 3])
    for subset in subsets:
        print(subset)
    
    print("\n=== N-QUEENS (4x4) ===")
    print("4-Queens solutions:")
    solutions = solve_n_queens(4)
    for i, solution in enumerate(solutions):
        print(f"Solution {i + 1}:")
        for row in solution:
            print(row)
        print()
    
    print("=== WORD SEARCH ===")
    board = [
        ['A','B','C','E'],
        ['S','F','C','S'],
        ['A','D','E','E']
    ]
    print(f"Board: {board}")
    print(f"Word 'ABCCED' exists: {word_search(board, 'ABCCED')}")
    print(f"Word 'SEE' exists: {word_search(board, 'SEE')}")
    print(f"Word 'ABCB' exists: {word_search(board, 'ABCB')}")
    
    print("\n=== COMBINATION SUM ===")
    print("Combinations that sum to 7 using [2,3,6,7]:")
    combinations = combination_sum([2,3,6,7], 7)
    for combo in combinations:
        print(combo)
    
    print("\n=== GENERATE PARENTHESES ===")
    print("Well-formed parentheses for n=3:")
    parentheses = generate_parentheses(3)
    for paren in parentheses:
        print(paren)


"""
WHEN TO USE BACKTRACKING:
========================

1. **Constraint Satisfaction Problems**: Sudoku, N-Queens, Graph Coloring
2. **Combinatorial Search**: Permutations, Combinations, Subsets
3. **Optimization Problems**: Finding the best path, optimal arrangement
4. **Parsing Problems**: Regular expression matching, parsing expressions
5. **Game Solving**: Solving puzzles, finding winning strategies

OPTIMIZATION TECHNIQUES:
=======================

1. **Pruning**: Cut off branches early when you know they can't lead to a solution
2. **Constraint Propagation**: Use constraints to reduce the search space
3. **Ordering Heuristics**: Choose the order of exploring candidates wisely
4. **Memoization**: Cache results of subproblems (when applicable)
5. **Early Termination**: Stop as soon as you find the first solution (if only one needed)

TIME COMPLEXITY ANALYSIS:
========================

- Generally exponential: O(b^d) where b is branching factor and d is depth
- Can be optimized with good pruning strategies
- Space complexity is usually O(d) for the recursion stack

COMMON MISTAKES:
===============

1. **Forgetting to backtrack**: Not undoing changes after recursive calls
2. **Modifying shared state**: Accidentally modifying data structures that are shared
3. **Infinite recursion**: Not having proper base cases
4. **Inefficient pruning**: Not cutting off invalid branches early enough
5. **Deep copying issues**: Forgetting to make copies when adding to results
"""
