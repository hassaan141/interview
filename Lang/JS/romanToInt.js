var romanToInt = function(s) {
  let score=0;
  let letter=['V','X', 'L', 'C', 'D', 'M']
  let arr={'I':1, 'V':5,'X':10, 'L':50, 'C':100, 'D':500, 'M':1000}
  let skip =false

  for (let i = 0; i<s.length; i++){
      if(skip==false){
          if(s[i]  =='I' && (s[i+1]=='V' || s[i+1]=='X')){
              score += (arr[s[i+1]]-1)
              skip =true
          }else if(s[i]  =='X' && (s[i+1]=='L' || s[i+1]=='C')){
              score += (arr[s[i+1]]-10)
              skip =true
          }else if(s[i]  =='C' && (s[i+1]=='D' || s[i+1]=='M')){
              score += (arr[s[i+1]]-100)
              skip =true
          }else{
              score += arr[s[i]]
          }
      }else{
          skip=false
      }
  }
  return score
};