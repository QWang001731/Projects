def getValidWord(s, dictionary):
    for word in dictionary:
        len_word = len(word)
        count = 0
        for c in word:
            if c in s:
                count += 1
        if count == len_word:
            return word     
    return -1


print(getValidWord("hgferyjkllkkop", ["coffer","hello","happy"]))

def findMinimumCoins(price, req, discount):
    price = sorted(price, reverse=True)
    req = sorted(req, reverse=False)
    discount = sorted(discount, reverse=True)
    sum = 0
    used = [False for _ in range(len(req))]
    for idx, d in enumerate(discount):
        for p in price:
            if p >= req[idx] and used[idx] is False:
                sum += d
                used[idx] = True

    sum1 = 0
    for p in price:
        sum1 += p
    
    return sum1 - sum
        

a= [2,8,3,7]
b= [8,7]
c=[5,4]

print(findMinimumCoins(a,b,c))


def findMaximumShorts(arr, l, r):
    result = []
    for i in range(len(l)):
        tem_arr = [a for _ in arr]
        left = l[i]
        right = r[i]
        for idx, a in enumerate(arr):
            sum = 0
            if left<= a and right>=a:
                sum+=1
                arr.pop(idx)
        for 
        
        

def range_test(arr, l, r):
    n = len(arr)
    in_it = 0
    for a in arr:
        if a>=l and a <= r:
            in_it += 1

    out = n -in_it
    if in_it > out:
        return True
    return False 


