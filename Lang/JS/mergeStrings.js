/**
 * @param {string} word1
 * @param {string} word2
 * @return {string}
 */
var mergeAlternately = function(word1, word2) {
  longer = ''
  shorter=''
  merge = ''
  if (word1.length>word2.length){
      longer = word1
      shorter = word2
  }else{
      longer = word2 
      shorter = word1
  }

  for(let i = 0; i<shorter.length; i++){
      merge = merge.concat(word1[i])
      merge = merge.concat(word2[i])
  }
  merge = merge.concat(longer.substring(shorter.length))
  console.log(merge)
  return merge
};
