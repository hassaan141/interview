/**
 * @param {number[]} nums
 * @return {number[]}
 */
var runningSum = function(nums) {

  let newArr = []
  for (let i = 0; i<nums.length; i++){

      let sum = 0;
      for (let j = 1; j<i; j++){

          sum += nums[j];
      }
         newArr.push(sum)
      
  }
  return newArr;
};