"""
Recursive call stack
"""

def fib(n):
    if n == 0:
        return 0
    elif n == 1:
        return 1
    
    else:
        return fib(n - 1) + fib(n - 2)
print(f"Recursive case is {fib(7)}")

"""
Top down memoization
"""

def fib2(n, check):

    if check[n] != -1:
        return check[n]
    
    if n == 0:
        check[n] = 0
        return check[n]
    elif n == 1:
        check[n] = 1
        return check[n]
    
    check[n] = fib2(n - 1, check) + fib2(n - 2, check)
    return check[n]

n = 7
check = [-1] * (n + 1)
print(check)
print(fib2(n, check))

"""
Bottom up tabulization
"""

def fib3(n):
    
    check = [0] * (n + 1)

    check[0] = 0
    check[1] = 1

    for i in range(2, n+1):
        check[i] = check[i-1] + check[i -2]
    
    return check[i]

print(fib3(7))

"""
top down no extra space
"""

def fib4(n):
    
    a, b = 0, 1

    for _ in range(2, n+1):
        
        temp = b
        b = a + b
        a = temp
    
    return b

print(fib4(7))


"""
Grid Traveller:

Given:

    Given an n x m grid, 
    Start top left, end bottom right
    can only move right and down

Task:

how many ways can you get to the goal

Questions: Backtracking? DP? 

Base case: If row or col is 0, impossible, canot be traveled
If Start == End; Reached goal 1x1

1x2; right

2x1; down

2x2; 1) right down 2) down right

2x3 1) right right down 2) down right right 3) right down right

3x2 1) right down down 2) down right down 3) down down right
"""

print('\nGrid Travel Question')

def travelGrid(n, m, memo={}):

    if (n, m) in memo:
        return memo[(n,m)]
    
    if n == 0 or m == 0:
        return 0
    if n == m == 1:
        return 1
    
    memo[(n, m)] = travelGrid(n - 1, m, memo) + travelGrid(n, m - 1, memo)
    return memo[(n,m)]
 
print(travelGrid(1, 1))
print(travelGrid(2, 3))
print(travelGrid(180, 108))

"""
Memoization Recipe

1. Make the recursive solution work
2. Make it efficent

"""


"""
Practice Problems

Solve these using recursion first, then memoization/tabulation.
"""

"""
    Question:
    Given a targetSum and a list of positive numbers,
    return True if targetSum can be generated using numbers from the list.
    You may use elements as many times as needed.
    Return False otherwise.

    Input: 7, [2, 3, 4]

            7 target
      [0]  /  [1] |   \ [2]
         5.      4.    3       

    Ouput: True or False
"""
def canSum(targetSum, numbers, memo={}):
    
    if targetSum < 0:
        return False
    if targetSum == 0:
        return True
        
    for i in numbers:
        remainder = targetSum-i
        if canSum(remainder, numbers, memo):
            memo[targetSum] = True
            return True
        
    return False
# def canSum(targetSum, numbers, memo={}):

#     if targetSum in memo:
#         return memo[targetSum]
    
#     if targetSum < 0:
#         return False
#     if targetSum == 0:
#         return True
        
#     for i in numbers:
#         remainder = targetSum-i
#         if canSum(remainder, numbers, memo):
#             memo[targetSum] = True
#             return True
        
#     memo[targetSum] = False
#     return False

print("\nCan Sum Solution")
# canSum tests (uncomment after implementation)
print(canSum(7, [2, 3], {}))                 # True
print(canSum(7, [5, 3, 4, 7], {}))           # True
print(canSum(7, [2, 4], {}))                 # False
print(canSum(8, [2, 3, 5], {}))              # True
print(canSum(300, [7, 14], {}))              # False

"""
    Question:
    Given a targetSum and a list of positive numbers,
    return any one combination of numbers that adds up to targetSum.
    If there is no valid combination, return None.

    Similar to canSum
    - (7, [2, 3]))  => [2, 2, 3]            

            7 
        /      \
       5       4  
      / \.    / \ 
     3  2    2. 1 
       
"""

print("\nHow Sum")
def howSum(targetSum, numbers) -> list[int]:

    def dfs(targetSum, numbers, out):

        if targetSum < 0:
            out = []
            return
        
        if targetSum == 0:
            out.append(-1)
            return 
        
        for i in numbers:
            remain = targetSum - i
            out.append(i)
            dfs(remain, numbers, out)
        
        return remain

    out = []
    dfs(targetSum, numbers, out)
    print(out)




# howSum tests (uncomment after implementation)
print(howSum(7, [2, 3]))                 # [3,2,2] or [2,2,3] etc.
# print(howSum(7, [5, 3, 4, 7]))           # [7] or [4,3] etc.
# print(howSum(7, [2, 4]))                 # None
# print(howSum(8, [2, 3, 5]))              # e.g. [2,2,2,2] or [3,5]
# print(howSum(300, [7, 14]))              # None


def bestSum(targetSum, numbers):
    """
    Question:
    Given a targetSum and a list of positive numbers,
    return the shortest combination of numbers that adds up to targetSum.
    If there is no valid combination, return None.
    """
    pass


# bestSum tests (uncomment after implementation)
# print(bestSum(7, [5, 3, 4, 7]))          # [7]
# print(bestSum(8, [2, 3, 5]))             # [3,5]
# print(bestSum(8, [1, 4, 5]))             # [4,4]
# print(bestSum(100, [1, 2, 5, 25]))       # [25,25,25,25]


def canConstruct(target, wordBank):
    """
    Question:
    Given a target string and a list of strings (wordBank),
    return True if the target can be constructed by concatenating
    elements of wordBank. You may reuse words any number of times.
    """
    pass


# canConstruct tests (uncomment after implementation)
# print(canConstruct('abcdef', ['ab', 'abc', 'cd', 'def', 'abcd']))                      # True
# print(canConstruct('skateboard', ['bo', 'rd', 'ate', 't', 'ska', 'sk', 'boar']))        # False
# print(canConstruct('enterapotentpot', ['a', 'p', 'ent', 'enter', 'ot', 'o', 't']))      # True
# print(canConstruct('eeeeeeeeeeeeeeeeeeeeef', ['e', 'ee', 'eee', 'eeee', 'eeeee']))      # False


def countConstruct(target, wordBank):
    """
    Question:
    Given a target string and a list of strings (wordBank),
    return the total number of ways the target can be constructed
    by concatenating elements of wordBank. You may reuse words.
    """
    pass


# countConstruct tests (uncomment after implementation)
# print(countConstruct('purple', ['purp', 'p', 'ur', 'le', 'purpl']))                      # 2
# print(countConstruct('abcdef', ['ab', 'abc', 'cd', 'def', 'abcd', 'ef', 'c']))          # 1
# print(countConstruct('skateboard', ['bo', 'rd', 'ate', 't', 'ska', 'sk', 'boar']))      # 0
# print(countConstruct('enterapotentpot', ['a', 'p', 'ent', 'enter', 'ot', 'o', 't']))    # 4

