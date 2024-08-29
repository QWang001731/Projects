import random
import math
LEN_OF_WORD = 32
DATA_SZ = 8


class CPU:
    def __init__(self, a, b, cacheParameters, X, Y, sz=DATA_SZ):
        self.X = X
        self.Y = Y
        self.cache_para = cacheParameters
        self.a = a
        self.b = b
        self.block_size = self.cache_para["block_size"]
        self.cache_size = self.cache_para["size"]
        self.associtivity = self.cache_para["associtivity"]
        self.write_mode = self.cache_para["write_mode"]
        self.cache = Cache(cacheParameters,
                           X, Y)
        
        self.counter = dict()
        self.counter["instruction_num"] = 0
        self.counter["read_misses"] = 0
        self.counter["read_hits"] = 0
        self.counter["write_misses"] = 0
        self.counter["write_hits"] = 0

    def clear_counter(self):
        self.counter["instruction_num"] = 0
        self.counter["read_misses"] = 0
        self.counter["read_hits"] = 0
        self.counter["write_misses"] = 0
        self.counter["write_hits"] = 0


    def loadDouble(self, addr):
        self.counter["instruction_num"] += 1
        if self.associtivity == 1:
            result, datablock = self.cache.getDouble_Dir_MAP(addr)
        elif self.cache_para["policy"] == "LRU":
            result, datablock = self.cache.getDouble_LRU(addr)
        elif self.cache_para["policy"] == "FIFO":
            result, datablock = self.cache.getDouble_FIFO(addr)
        elif self.cache_para["policy"] == "random":
            result, datablock = self.cache.getDouble_random(addr)

        if result == "hit":
            self.counter["read_hits"] += 1
        if result == "miss":
            self.counter["read_misses"] += 1
        add = Address(addr, self.cache_para)
        byte_num = int(add.getByteOff()//DATA_SZ)
        return datablock[byte_num]
    
    def storeDouble(self, addr, value):
        self.counter["instruction_num"] += 1 
        if self.associtivity == 1:
            result = self.cache.setDouble_Dir_MAP(addr, value)
        elif self.cache_para["policy"] == "LRU":
            result = self.cache.setDouble_LRU(addr, value)
        elif self.cache_para["policy"] == "FIFO":
            result = self.cache.setDouble_FIFO(addr, value)
        elif self.cache_para["policy"] == "random":
            result = self.cache.setDouble_random(addr, value)
        if result == "hit":
            self.counter["write_hits"] += 1
        if result == "miss":
            self.counter["write_misses"] += 1
        return
        
    def addDouble(self, value1, value2):
        self.counter["instruction_num"] += 1
        return value1 + value2
    
    def mulDouble(self, value1, value2):
        self.counter["instruction_num"] += 1 
        return value1 * value2

    def Daxpy(self):
        self.cache.flush_cache()
        self.clear_counter()
        data_size = len(self.X)
        register0 = self.a
        X_addr = list(range(0, data_size*DATA_SZ, DATA_SZ))
        Y_addr = list(range(data_size*DATA_SZ, data_size*DATA_SZ*2, DATA_SZ))
        Z_addr = list(range(data_size*2*DATA_SZ, data_size*DATA_SZ*3, DATA_SZ))
        Result = []
        for i in range(len(self.X)):
            register1 = self.loadDouble(X_addr[i])
            register2 = self.mulDouble(register0, register1)
            register3 = self.loadDouble(Y_addr[i])
            register4 = self.addDouble(register2, register3)
            self.storeDouble(Z_addr[i], register4)
            Result.append(register4)
        return Result
    
    def Dgemm(self):
        self.cache.flush_cache()
        self.clear_counter()
        data_size = int(len(self.X))
        X_addr = list(range(0, data_size*DATA_SZ, DATA_SZ))
        Y_addr = list(range(data_size*DATA_SZ, data_size*DATA_SZ*2, DATA_SZ))
        Z_addr = list(range(data_size*2*DATA_SZ, data_size*DATA_SZ*3, DATA_SZ))
        n = int(math.sqrt(data_size))
        rowX = [None for _ in range(n)]
        colY = [None for _ in range(n)]
        Z = [[0 for _ in range(n)] for _ in range(n)]
        R = []
        
        for i in range(n):
            start_x = i * n
            for ii in range(n):
                rowX[ii] = self.loadDouble(X_addr[start_x + ii])

            for j in range(n):
                start_y = j
                for jj in range(n):
                    colY[jj] = self.loadDouble(Y_addr[start_y + jj*n])
                
                for k in range(n):
                    register1 = self.mulDouble(rowX[k], colY[k])
                    Z[i][j] = self.addDouble(Z[i][j], register1)
            
                self.storeDouble(Z_addr[i * n + j], Z[i][j])
                R.append(Z[i][j])
        
        return R
    
    def DgemmBlock(self):
        self.cache.flush_cache()
        self.clear_counter()
        data_size = int(len(self.X))
        X_addr = list(range(0, data_size*DATA_SZ, DATA_SZ))
        Y_addr = list(range(data_size*DATA_SZ, data_size*DATA_SZ*2, DATA_SZ))
        Z_addr = list(range(data_size*2*DATA_SZ, data_size*DATA_SZ*3, DATA_SZ))
        b = self.b
        n = int(math.sqrt(data_size))
        N = int(n/b)
        subX = [[None for _ in range(b)] for _ in range(b)] 
        subY = [[None for _ in range(b)] for _ in range(b)] 
        
        R = [None for _ in range(n * n)]
        print(f"n:{n} N:{N}, b:{b}")
        for i in range(N):
            for j in range(N):
                subZ = [[0 for _ in range(b)] for _ in range(b)]
                
                for k in range(N):
                    start_x = i * b * n + k * b
                    start_y = j * b + k * b * n
                    
                    for p in range(b):
                        for q in range(b):
                            add_x = X_addr[start_x + p * n + q]
                            subX[p][q] = self.loadDouble(add_x)
                            add_y = Y_addr[start_y + p * n + q]
                            subY[p][q] = self.loadDouble(add_y)
                    
                    for p in range(b):
                        for q in range(b):
                            register1 = 0
                            for r in range(b):
                                register0 = self.mulDouble(subX[p][r], subY[r][q])
                                register1 = self.addDouble(register1, register0)
                            subZ[p][q] = self.addDouble(subZ[p][q], register1)

                for pp in range(b):
                    for qq in range(b):
                        add_z = Z_addr[(i * b + pp) * n + j * b + qq]
                        self.storeDouble(add_z, subZ[pp][qq])
                        R[(i * b + pp) * n + j * b + qq] = subZ[pp][qq]
        return R


class Address:
    def __init__(self, addr, cache_para) -> None:
        self.addr = addr
        cache_size = cache_para["size"]
        block_size = cache_para["block_size"]
        self.associtivity = cache_para["associtivity"]
        self.blockOffsetLen = int(math.log2(self.associtivity))
        self.byteOffsetLen = int(math.log2(block_size))
        self.indexLen = int(math.log2(cache_size)) - self.blockOffsetLen - self.byteOffsetLen
        self.tagLen = LEN_OF_WORD - self.indexLen-self.blockOffsetLen-self.byteOffsetLen
    
    def getTag(self):
        addr_bin = bin(self.addr)[2:]
        addr_bin = addr_bin.zfill(LEN_OF_WORD)
        addr_bin = addr_bin[:self.tagLen]
        self.tag = addr_bin
        addr_dec = int(addr_bin, 2)
        return addr_dec
    
    def getIndex(self):
        addr_bin = bin(self.addr)[2:]
        addr_bin = addr_bin.zfill(LEN_OF_WORD)
        start = self.tagLen
        end = start + self.indexLen 
        if start == end:
            addr_dec = 0
        else:
            addr_bin = addr_bin[start:end]
            self.index = addr_bin
            addr_dec = int(addr_bin, 2)
        return addr_dec
    
    def getBlockOff(self):
        if self.associtivity > 1:
            addr_bin = bin(self.addr)[2:]
            addr_bin = addr_bin.zfill(LEN_OF_WORD)
            start = self.indexLen + self.tagLen
            end = start + self.blockOffsetLen
            addr_bin = addr_bin[start:end]
            addr_dec = int(addr_bin, 2)
            return addr_dec
        else:
            return 0
    
    def printAdd(self):
        print(f"tag:{self.getTag()}, index:{self.getIndex()},blockoff:{self.getBlockOff()}, byteoff:{self.getByteOff()}")
    
    def getByteOff(self):
        addr_bin = bin(self.addr)[2:]
        addr_bin = addr_bin.zfill(LEN_OF_WORD)
        start = self.indexLen + self.tagLen + self.blockOffsetLen
        end = start + self.byteOffsetLen
        addr_bin = addr_bin[start:end]
        addr_dec = int(addr_bin, 2)
        return addr_dec


class Cache:
    def __init__(self, cache_para, X, Y
                 ) -> None:
        self.cache_para = cache_para
        cache_size = cache_para["size"]
        blockSize = cache_para["block_size"]
        associtivity = cache_para["associtivity"]
        numBlocks = int(cache_size/blockSize)
        self.numSets = int(numBlocks/associtivity)
        self.associtivity = associtivity
        self.dataBlock = [[None for _ in range(self.associtivity)] for _ in
                          range(self.numSets)]
        self.valid = [[False for _ in range(self.associtivity)] for
                      _ in range(self.numSets)]
        self.dirty = [[False for _ in range(self.associtivity)] for
                      _ in range(self.numSets)]
        self.tag = [[None for _ in range(self.associtivity)] for
                    _ in range(self.numSets)]
        self.blockOff = [[None for _ in range(self.associtivity)] for
                            _ in range(self.numSets)]
        self.LRU_trace_number = [[0 for _ in range(self.associtivity)] for _ in 
                                 range(self.numSets)]
        self.FIFO_trace_number = [[0 for _ in range(self.associtivity)] for _ in
                                  range(self.numSets)]
        self.Ram = RAM(blockSize, X, Y)
    
    def flush_cache(self):
        numSets = self.numSets
        associativity = self.associtivity
        for i in range(numSets):
            for j in range(associativity):
                if self.valid[i][j] is True:
                    self.valid[i][j] = False


    def getDouble_Dir_MAP(self, addr):
        add = Address(addr, self.cache_para)
        tag = add.getTag()
        indx = add.getIndex()
        block_off = add.getBlockOff()
        
        if not self.valid[indx][block_off]:
            self.dataBlock[indx][block_off] = self.Ram.getBlock(addr)
            self.tag[indx][block_off] = tag
            self.valid[indx][block_off] = True
            self.dirty[indx][block_off] = False
            return "miss", [double for double in self.dataBlock[indx][block_off]]

        elif self.tag[indx][block_off] == tag:
            return "hit", [double for double in self.dataBlock[indx][block_off]]
        
        else:
            if self.dirty[indx][block_off]:
                add = Address(addr, self.cache_para)
                tag_len = add.tagLen
                old_tag_bin = bin(self.tag[indx][block_off])[2:].zfill(tag_len)
                indx_bin = bin(addr)[2:].zfill(LEN_OF_WORD)[tag_len:]
                add_bin = old_tag_bin + indx_bin
                add_dec = int(add_bin, 2)
                self.Ram.setBlock(add_dec, [double for double in self.dataBlock[indx][block_off]])
            self.dataBlock[indx][block_off] = [double for double in self.Ram.getBlock(addr)]
            self.dirty[indx][block_off] = False
            return "miss", [double for double in self.dataBlock[indx][block_off]]

    def getDouble_LRU(self, addr):
        add = Address(addr, self.cache_para)
        tag = add.getTag()
        indx = add.getIndex()
        block_off = add.getBlockOff()
        index_in_set = None
        associtivity = self.associtivity
        for i in range(associtivity):
            if self.valid[indx][i] is True and self.tag[indx][i] == tag and \
                self.blockOff[indx][i] == block_off:
                index_in_set = i
        if index_in_set is not None:
            curr_max_time = max(self.LRU_trace_number[indx])
            self.LRU_trace_number[indx][index_in_set] = curr_max_time + 1
            return "hit", [double for double in self.dataBlock[indx][index_in_set]]
        else:
            data_block = self.Ram.getBlock(addr)
            for i in range(associtivity):
                if self.valid[indx][i] is False:
                    index_in_set = i
                    self.dataBlock[indx][index_in_set] = [double for double in data_block]
                    self.tag[indx][index_in_set] = tag
                    self.blockOff[indx][index_in_set] = block_off
                    self.valid[indx][index_in_set] = True
                    self.dirty[indx][index_in_set] = False
                    break

            if index_in_set is None:
                curr_min_time = min(self.LRU_trace_number[indx])
                to_evict = self.LRU_trace_number[indx].index(curr_min_time)
                if self.dirty[indx][to_evict] is True:
                    s = ""
                    for i in range(add.byteOffsetLen):
                        s = s + "0"
                    tag1 = self.tag[indx][to_evict]
                    
                    block_off1 = self.blockOff[indx][to_evict]
                    if add.indexLen > 0:
                        add_bin = bin(tag1)[2:].zfill(add.tagLen) + bin(indx)[2:].zfill(add.indexLen) +\
                            bin(block_off1)[2:].zfill(add.blockOffsetLen) + s
                    else:
                        add_bin = bin(tag1)[2:].zfill(add.tagLen) + bin(block_off1)[2:].zfill(add.blockOffsetLen) + s

                    add_dec = int(add_bin, 2)
                    self.Ram.setBlock(add_dec, [double for double in self.dataBlock[indx][to_evict]])
                
                index_in_set = to_evict
                self.dataBlock[indx][index_in_set] = [double for double in data_block]
                self.tag[indx][index_in_set] = tag
                self.blockOff[indx][index_in_set] = block_off
                self.valid[indx][index_in_set] = True
                self.dirty[indx][index_in_set] = False
          
            curr_max_time = max(self.LRU_trace_number[indx])
            self.LRU_trace_number[indx][index_in_set] = curr_max_time + 1
            return "miss", [double for double in self.dataBlock[indx][index_in_set]]
                

    def getDouble_FIFO(self, addr):
        add = Address(addr, self.cache_para)
        tag = add.getTag()
        indx = add.getIndex()
        block_off = add.getBlockOff()
        associtivity = self.associtivity
        index_in_set = None
        for i in range(associtivity):
            if self.valid[indx][i] is True and self.tag[indx][i] == tag and \
                self.blockOff[indx][i] == block_off:
                index_in_set = i
        if index_in_set is not None:
            return "hit", [double for double in self.dataBlock[indx][index_in_set]]
        else:
            data_block = self.Ram.getBlock(addr)
            for i in range(associtivity):
                if self.valid[indx][i] is False:
                    index_in_set = i
                    self.dataBlock[indx][index_in_set] = [double for double in data_block]
                    self.tag[indx][index_in_set] = tag
                    self.blockOff[indx][index_in_set] = block_off
                    self.valid[indx][index_in_set] = True
                    self.dirty[indx][index_in_set] = False
                    break

            if index_in_set is None:
                curr_min_time = min(self.FIFO_trace_number[indx])
                to_evict = self.FIFO_trace_number[indx].index(curr_min_time)
                if self.dirty[indx][to_evict] is True:
                    s = ""
                    for i in range(add.byteOffsetLen):
                        s = s + "0"
                    tag1 = self.tag[indx][to_evict]
                    
                    block_off1 = self.blockOff[indx][to_evict]
                    if add.indexLen > 0:
                        add_bin = bin(tag1)[2:].zfill(add.tagLen) + bin(indx)[2:].zfill(add.indexLen) +\
                            bin(block_off1)[2:].zfill(add.blockOffsetLen) + s
                    else:
                        add_bin = bin(tag1)[2:].zfill(add.tagLen) + bin(block_off1)[2:].zfill(add.blockOffsetLen) + s

                    add_dec = int(add_bin, 2)
                    self.Ram.setBlock(add_dec, [double for double in self.dataBlock[indx][to_evict]])

                index_in_set = to_evict
                self.dataBlock[indx][index_in_set] = [double for double in data_block]
                self.tag[indx][index_in_set] = tag
                self.blockOff[indx][index_in_set] = block_off
                self.valid[indx][index_in_set] = True
                self.dirty[indx][index_in_set] = False

            curr_max_time = max(self.FIFO_trace_number[indx])
            self.FIFO_trace_number[indx][index_in_set] = curr_max_time + 1
            return "miss", [double for double in self.dataBlock[indx][index_in_set]]

    def getDouble_random(self, addr):
        add = Address(addr, self.cache_para)
        tag = add.getTag()
        indx = add.getIndex()
        block_off = add.getBlockOff()
        associtivity = self.associtivity
        index_in_set = None
        for i in range(associtivity):
            if self.valid[indx][i] is True and self.tag[indx][i] == tag and \
                self.blockOff[indx][i] == block_off:
                index_in_set = i

        if index_in_set is not None:
            return "hit", [double for double in self.dataBlock[indx][index_in_set]]

        else:
            data_block = self.Ram.getBlock(addr)
            for i in range(associtivity):
                if self.valid[indx][i] is False:
                    index_in_set = i
                    self.dataBlock[indx][index_in_set] = [double for double in data_block]
                    self.tag[indx][index_in_set] = tag
                    self.blockOff[indx][index_in_set] = block_off
                    self.valid[indx][index_in_set] = True
                    self.dirty[indx][index_in_set] = False
                    break
            
            if index_in_set is None:
                to_evict = random.randint(0, associtivity-1)
                if self.dirty[indx][to_evict] is True:
                    s = ""
                    for i in range(add.byteOffsetLen):
                        s = s + "0"
                    tag1 = self.tag[indx][to_evict]
                    block_off1 = self.blockOff[indx][to_evict]
                    if add.indexLen > 0:
                        add_bin = bin(tag1)[2:].zfill(add.tagLen) + bin(indx)[2:].zfill(add.indexLen) +\
                            bin(block_off1)[2:].zfill(add.blockOffsetLen) + s
                    else:
                        add_bin = bin(tag1)[2:].zfill(add.tagLen) + bin(block_off1)[2:].zfill(add.blockOffsetLen) + s

                    add_dec = int(add_bin, 2)
                    self.Ram.setBlock(add_dec, [double for double in self.dataBlock[indx][to_evict]])

                index_in_set = to_evict
                self.dataBlock[indx][index_in_set] = [double for double in data_block]
                self.tag[indx][index_in_set] = tag
                self.blockOff[indx][index_in_set] = block_off
                self.valid[indx][index_in_set] = True
                self.dirty[indx][index_in_set] = False
            return "miss", [double for double in self.dataBlock[indx][index_in_set]]
        
    def setDouble_Dir_MAP(self, addr, value):
        add = Address(addr, self.cache_para)
        write_mode = self.cache_para["write_mode"]
        tag = add.getTag()
        index = add.getIndex()
        blockOff = add.getBlockOff()
        byteoff = add.getByteOff()
        avaiable_slot = None
        if self.valid[index][blockOff] is False:
            avaiable_slot = 0

        if avaiable_slot is not None:
            data_block = []
            for double in self.Ram.getBlock(addr):
                data_block.append(double)
            
            index_in_block = int(byteoff/DATA_SZ)
            data_block[index_in_block] = value
            self.dataBlock[index][blockOff] = [double for double in data_block]
            self.valid[index][blockOff] = True
            self.dirty[index][blockOff] = True
            self.tag[index][blockOff] = tag
            if write_mode == "write_through":
                self.Ram.setBlock(addr, [double for double in self.dataBlock[index][blockOff]])
            return "miss"

        elif self.tag[index][blockOff] == tag:
            index_in_block = int(byteoff/DATA_SZ)
            self.dataBlock[index][blockOff][index_in_block] = value
            self.valid[index][blockOff] = True
            self.dirty[index][blockOff] = True
            if write_mode == "write_through":
                self.Ram.setBlock(addr, [double for double in self.dataBlock[index][blockOff]])
            return "hit"
        else:
            if self.dirty[index][blockOff]:
                add = Address(addr, self.cache_para)
                tag_len = add.tagLen
                old_tag_bin = bin(self.tag[index][blockOff])[2:].zfill(tag_len)
                indx_bin = bin(addr)[2:].zfill(LEN_OF_WORD)[tag_len:]
                add_bin = old_tag_bin + indx_bin
                add_dec = int(add_bin, 2)
                self.Ram.setBlock(add_dec, [double for double in self.dataBlock[index][blockOff]])

            data_block = [double for double in self.Ram.getBlock(addr)]
            index_in_block = int(byteoff/DATA_SZ)
            data_block[index_in_block] = value
            self.dataBlock[index][blockOff] = [double for double in data_block]
            self.valid[index][blockOff] = True
            self.dirty[index][blockOff] = True
            self.tag[index][blockOff] = tag
            
            if write_mode == "write_through":
                self.Ram.setBlock(addr, [double for double in self.dataBlock[index][blockOff]])
            return "miss"

    def setDouble_LRU(self, addr, value):
        add = Address(addr, self.cache_para)
        tag = add.getTag()
        index = add.getIndex()
        blockOff = add.getBlockOff()
        byteoff = add.getByteOff()
        write_mode = self.cache_para["write_mode"]
        associtivity = self.cache_para["associtivity"]
        write_slot = None
        for i in range(associtivity):
            if self.valid[index][i] is True and self.tag[index][i] == tag and \
                self.blockOff[index][i] == blockOff:
                write_slot = i
                break

        if write_slot is not None:
            index_in_block = int(byteoff/DATA_SZ)
            self.dataBlock[index][write_slot][index_in_block] = value
            self.dirty[index][write_slot] = True
            self.valid[index][write_slot] = True
            self.tag[index][write_slot] = tag
            self.blockOff[index][write_slot] = blockOff
            if write_mode == "write_through":
                self.Ram.setBlock(addr, [double for double in
                                         self.dataBlock[index][write_slot]])
                
            curr_max_time = max(self.LRU_trace_number[index])
            self.LRU_trace_number[index][write_slot] = curr_max_time + 1
            return "hit"
        else:
            for i in range(associtivity):
                if self.valid[index][i] == False:
                    write_slot = i
                    break
            if write_slot is not None:
                data_block = [double for double in self.Ram.getBlock(addr)]
                index_in_block = int(byteoff/DATA_SZ)
                data_block[index_in_block] = value
                self.dataBlock[index][write_slot] = [double for double in data_block]
                self.valid[index][write_slot] = True
                self.dirty[index][write_slot] = True
                self.tag[index][write_slot] = tag
                self.blockOff[index][write_slot] = blockOff
                if write_mode == "write_through":
                    self.Ram.setBlock(addr, [double for double in self.
                                             dataBlock[index][write_slot]])
                curr_max_time = max(self.LRU_trace_number[index])
                self.LRU_trace_number[index][write_slot] = curr_max_time + 1
                return "miss"
            else:
                curr_min_time = min(self.LRU_trace_number[index])
                to_evict = self.LRU_trace_number[index].index(curr_min_time)
                if self.dirty[index][to_evict] is True:
                    s = ""
                    for i in range(add.byteOffsetLen):
                        s = s + "0"
                    tag1 = self.tag[index][to_evict]
                    
                    block_off1 = self.blockOff[index][to_evict]
                    if add.indexLen > 0:
                        add = bin(tag1)[2:].zfill(add.tagLen) + bin(index)[2:].zfill(add.indexLen) +\
                            bin(block_off1)[2:].zfill(add.blockOffsetLen) + s
                    else:
                        add = bin(tag1)[2:].zfill(add.tagLen) + bin(block_off1)[2:].zfill(add.blockOffsetLen) + s
        
                    add_dec = int(add, 2)
                    self.Ram.setBlock(add_dec, [double for double in self.
                                                dataBlock[index][to_evict]])
                    
                write_slot = to_evict
                data_block = [double for double in self.Ram.getBlock(addr)]
                index_in_block = int(byteoff/DATA_SZ)
                data_block[index_in_block] = value
                self.dataBlock[index][write_slot] = [double for double in
                                                     data_block]
                self.valid[index][write_slot] = True
                self.dirty[index][write_slot] = True
                self.tag[index][write_slot] = tag
                self.blockOff[index][write_slot] = blockOff
                if write_mode == "write_through":
                    self.Ram.setBlock(addr, [double for double in
                                             self.dataBlock[index][write_slot]])
                curr_max_time = max(self.LRU_trace_number[index])
                self.LRU_trace_number[index][write_slot] = curr_max_time + 1
                return "miss"
        
    def setDouble_FIFO(self, addr, value):
        add = Address(addr, self.cache_para)
        tag = add.getTag()
        index = add.getIndex()
        blockOff = add.getBlockOff()
        byteoff = add.getByteOff()
        write_mode = self.cache_para["write_mode"]
        associtivity = self.cache_para["associtivity"]
        write_slot = None
        
        for i in range(associtivity):
            if self.valid[index][i] is True and self.tag[index][i] == tag and \
                self.blockOff[index][i] == blockOff:
                write_slot = i
                break

        if write_slot is not None:
            index_in_block = int(byteoff/DATA_SZ)
            self.dataBlock[index][write_slot][index_in_block] = value
            self.dirty[index][write_slot] = True
            self.valid[index][write_slot] = True
            self.tag[index][write_slot] = tag
            self.blockOff[index][write_slot] = blockOff
            if write_mode == "write_through":
                self.Ram.setBlock(addr, [double for double in
                                         self.dataBlock[index][write_slot]])
                
            return "hit"
        else:
            for i in range(associtivity):
                if self.valid[index][i] == False:
                    write_slot = i
                    break
            if write_slot is not None:
                data_block = [double for double in self.Ram.getBlock(addr)]
                index_in_block = int(byteoff/DATA_SZ)
                data_block[index_in_block] = value
                self.dataBlock[index][write_slot] = [double for double in data_block]
                self.valid[index][write_slot] = True
                self.dirty[index][write_slot] = True
                self.tag[index][write_slot] = tag
                self.blockOff[index][write_slot] = blockOff
                if write_mode == "write_through":
                    self.Ram.setBlock(addr, [double for double in self.
                                             dataBlock[index][write_slot]])
                curr_max_time = max(self.FIFO_trace_number[index])
                self.FIFO_trace_number[index][write_slot] = curr_max_time + 1
                return "miss"
            else:
                curr_min_time = min(self.FIFO_trace_number[index])
                to_evict = self.FIFO_trace_number[index].index(curr_min_time)
                if self.dirty[index][to_evict] is True:
                    s = ""
                    for i in range(add.byteOffsetLen):
                        s = s + "0"
                    tag1 = self.tag[index][to_evict]
                    block_off1 = self.blockOff[index][to_evict]
                    if add.indexLen > 0:
                        add = bin(tag1)[2:].zfill(add.tagLen) + bin(index)[2:].zfill(add.indexLen) +\
                            bin(block_off1)[2:].zfill(add.blockOffsetLen) + s
                    else:
                        add = bin(tag1)[2:].zfill(add.tagLen) + bin(block_off1)[2:].zfill(add.blockOffsetLen) + s
                    add_dec = int(add, 2)
                    self.Ram.setBlock(add_dec, [double for double in self.
                                                dataBlock[index][to_evict]])
                    
                write_slot = to_evict
                data_block = [double for double in self.Ram.getBlock(addr)]
                index_in_block = int(byteoff/DATA_SZ)
                data_block[index_in_block] = value
                self.dataBlock[index][write_slot] = [double for double in
                                                     data_block]
                self.valid[index][write_slot] = True
                self.dirty[index][write_slot] = True
                self.tag[index][write_slot] = tag
                self.blockOff[index][write_slot] = blockOff
                if write_mode == "write_through":
                    self.Ram.setBlock(addr, [double for double in
                                             self.dataBlock[index][write_slot]])
                    
                curr_max_time = max(self.FIFO_trace_number[index])
                self.FIFO_trace_number[index][write_slot] = curr_max_time + 1
                return "miss"
            
    def setDouble_random(self, addr, value):
        add = Address(addr, self.cache_para)
        tag = add.getTag()
        index = add.getIndex()
        blockOff = add.getBlockOff()
        byteoff = add.getByteOff()
        write_mode = self.cache_para["write_mode"]
        associtivity = self.cache_para["associtivity"]
        write_slot = None
        
        for i in range(associtivity):
            if self.valid[index][i] is True and self.tag[index][i] == tag and \
                self.blockOff[index][i] == blockOff:
                write_slot = i
                break

        if write_slot is not None:
            index_in_block = int(byteoff/DATA_SZ)
            self.dataBlock[index][write_slot][index_in_block] = value
            self.dirty[index][write_slot] = True
            self.valid[index][write_slot] = True
            self.tag[index][write_slot] = tag
            self.blockOff[index][write_slot] = blockOff
            if write_mode == "write_through":
                self.Ram.setBlock(addr, [double for double in
                                         self.dataBlock[index][write_slot]])
                
            return "hit"
        else:
            for i in range(associtivity):
                if self.valid[index][i] == False:
                    write_slot = i
                    break
            if write_slot is not None:
                data_block = [double for double in self.Ram.getBlock(addr)]
                index_in_block = int(byteoff/DATA_SZ)
                data_block[index_in_block] = value
                self.dataBlock[index][write_slot] = [double for double in data_block]
                self.valid[index][write_slot] = True
                self.dirty[index][write_slot] = True
                self.tag[index][write_slot] = tag
                self.blockOff[index][write_slot] = blockOff
                if write_mode == "write_through":
                    self.Ram.setBlock(addr, [double for double in self.
                                             dataBlock[index][write_slot]])
                return "miss"
            else:
                to_evict = random.randint(0, associtivity-1)
                if self.dirty[index][to_evict] is True:
                    s = ""
                    for i in range(add.byteOffsetLen):
                        s = s + "0"
                    tag1 = self.tag[index][to_evict]
                    block_off1 = self.blockOff[index][to_evict]
                    if add.indexLen > 0:
                        add = bin(tag1)[2:].zfill(add.tagLen) + bin(index)[2:].zfill(add.indexLen) +\
                            bin(block_off1)[2:].zfill(add.blockOffsetLen) + s
                    else:
                        add = bin(tag1)[2:].zfill(add.tagLen) + bin(block_off1)[2:].zfill(add.blockOffsetLen) + s
                    add_dec = int(add, 2)
                    self.Ram.setBlock(add_dec, [double for double in self.
                                                dataBlock[index][to_evict]])
                    
                write_slot = to_evict
                data_block = [double for double in self.Ram.getBlock(addr)]
                index_in_block = int(byteoff/DATA_SZ)
                data_block[index_in_block] = value
                self.dataBlock[index][write_slot] = [double for double in
                                                     data_block]
                self.valid[index][write_slot] = True
                self.dirty[index][write_slot] = True
                self.tag[index][write_slot] = tag
                self.blockOff[index][write_slot] = blockOff
                if write_mode == "write_through":
                    self.Ram.setBlock(addr, [double for double in
                                             self.dataBlock[index][write_slot]])
                    
                return "miss"


    def printCache(self):
        for i in range(self.numSets):
            for j in range(self.associtivity):
                print(self.valid[i][j], sep=" ")
            
class RAM:
    def __init__(self, blockSize, X, Y) -> None:
        self.blockSize = blockSize
        X_len = len(X)
        Z = [None for _ in range(X_len)]
        Memory = X + Y + Z
        num_of_doubles = len(Memory)
        doubles_per_block = self.blockSize/DATA_SZ
        numBlocks = int(num_of_doubles/(doubles_per_block)) + 1
        self.dataBlock = [[] for _ in range(numBlocks)]
        
        for idx, double in enumerate(Memory):
            i = idx // int(self.blockSize/DATA_SZ)
            self.dataBlock[i].append(double)

    def getBlock(self, addr):
        block_num = int(addr // self.blockSize)
        return self.dataBlock[block_num]
    
    def setBlock(self, addr, dataBlock):
        block_num = int(addr // self.blockSize)
        self.dataBlock[block_num] = dataBlock
        return