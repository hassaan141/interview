/**
 * @param {string} s
 * @return {number}
 */
var longestValidParentheses = function(s) {
    
  let arr = s.split('')
  let stack = []
  let count = 0;
  for (let i = 0; i<=arr.length; i++){

      if (arr[i]=='('){
          stack.push(arr[i])
      }
      else{
          if(stack.push() == '(' ){
              count +=2
          }
      }
  }

  return count;


};