package main

import (
	"bufio"
	"flag"
	"fmt"
	"os"
	"sort"
	"strconv"
	"strings"
	"sync"
	"time"
)

type BstNode struct {
	value int
	right *BstNode
	left  *BstNode
}

func (node *BstNode) insert(val int) {
	if val < node.value {
		if node.left == nil {
			node.left = new(BstNode)
			node.left.value = val
		} else {
			node.left.insert(val)
		}
	} else {
		if node.right == nil {
			node.right = new(BstNode)
			node.right.value = val
		} else {
			node.right.insert(val)
		}
	}
}

func (node *BstNode) print() {
	if node != nil {
		node.left.print()
		fmt.Printf("%d ", node.value)
		node.right.print()
	}
}

func (node *BstNode) genHash() int {
	var startingHash int = 1
	node.genHashRecursive(&startingHash)
	return startingHash
}

func (node *BstNode) genHashRecursive(currVal *int) {
	if node != nil {
		node.left.genHashRecursive(currVal)

		var newVal int = node.value + 2
		*currVal = (*currVal*newVal + newVal) % 1000

		node.right.genHashRecursive(currVal)
	}
}

func (node *BstNode) createArr(vals *[]int) {
	if node != nil {
		node.left.createArr(vals)
		*vals = append(*vals, node.value)
		node.right.createArr(vals)
	}
}

func (node *BstNode) compareTo(otherNode *BstNode) bool {
	nodeVals := []int{}
	otherNodeVals := []int{}
	node.createArr(&nodeVals)
	otherNode.createArr(&otherNodeVals)

	if len(nodeVals) != len(otherNodeVals) {
		return false
	}

	for i := range nodeVals {
		if nodeVals[i] != otherNodeVals[i] {
			return false
		}
	}
	return true
}

func main() {
	hashWorkers := flag.Int("hash-workers", 1, "Integer-valued number of threads")
	dataWorkers := flag.Int("data-workers", 1, "Integer-valued number of threads")
	compWorkers := flag.Int("comp-workers", 0, "Non-zero integer-valued number of threads")
	input := flag.String("input", "", "string-valued path to an input file")

	flag.Parse()

	if *input == "" {
		fmt.Printf("ERROR: No file specified.\n")
		// error
	}

	file, err := os.Open(*input)
	if err != nil {
		fmt.Printf("ERROR: Unable to read file %s\n", *input)
		// error
	}

	scanner := bufio.NewScanner(file)

	trees := []*BstNode{}
	var currIndex = 0
	for scanner.Scan() {
		var curr = scanner.Text()
		nums := strings.Fields(curr)

		for index := range nums {
			intNum, err := strconv.Atoi(nums[index])
			if err != nil {
				fmt.Printf("ERROR: Invalid Input. ")
				// error
			}
			if index == 0 {
				temp := new(BstNode)
				temp.value = intNum
				trees = append(trees, temp)
			} else {
				trees[currIndex].insert(intNum)
			}
		}
		// fmt.Printf("%s", curr)
		currIndex++
	}

	// Compare Hash Time code:
	var compareHashingTimes = false
	if compareHashingTimes {
		hashes := make([]int, len(trees))
		startHashTime := time.Now()
		computeHashes(&hashes, &trees)
		computeHashTime := time.Since(startHashTime)
		fmt.Printf("Time taken for nTrees goroutines: %s\n", computeHashTime)

		hashes = make([]int, len(trees))
		startHashTime = time.Now()
		computeHashesWithNumThreads(&hashes, &trees, *hashWorkers)
		computeHashTime = time.Since(startHashTime)
		fmt.Printf("Time taken for %d goroutines: %s\n", *hashWorkers, computeHashTime)
	}

	startTime := time.Now()

	// Compute Hashes
	hashesMap := make(map[int][]int)
	hashTime := computeHashesAndGroup(&trees, &hashesMap, *hashWorkers, *dataWorkers)

	hashGroupElapsedTime := time.Since(startTime)

	fmt.Printf("hashTime: %s\n", hashTime)
	fmt.Printf("hashGroupTime: %s\n", hashGroupElapsedTime)

	keys := make([]int, len(hashesMap))
	var i = 0
	for key := range hashesMap {
		keys[i] = key
		i++
	}
	sort.Ints(keys)

	for _, key := range keys {
		if len(hashesMap[key]) > 1 {
			fmt.Printf("%d:", key)
			for j := range hashesMap[key] {
				fmt.Printf(" %d", hashesMap[key][j])
			}
			fmt.Print("\n")
		}
	}
	if *compWorkers > 0 {
		// "Do not show any output from the tree comparison if -comp-workers is not specified"
		compareStart := time.Now()
		var result = [][]int{}
		if *compWorkers == 1 {
			compareTreesSeq(&trees, &hashesMap, &result, &keys)
		} else {
			compareTreesParallelWorkers(&trees, &hashesMap, &result, &keys, *compWorkers)
			// compareTreesParallel(&trees, &hashesMap, &result, &keys)
		}
		compareElapsedTime := time.Since(compareStart)

		fmt.Printf("compareTreeTime: %s\n", compareElapsedTime)
		var index = 0
		for i := range result {
			fmt.Printf("group %d:", index)
			for j := range result[i] {
				fmt.Printf(" %d", result[i][j])
			}
			fmt.Print("\n")
			index++
		}
	}
}

func computeHashes(hashes *[]int, trees *[]*BstNode) {
	computeHashesWithNumThreads(hashes, trees, len(*hashes))
}

func computeHashesWithNumThreads(hashes *[]int, trees *[]*BstNode, hashWorkers int) {
	var wg sync.WaitGroup
	wg.Add(hashWorkers)
	var divWork = len(*hashes) / hashWorkers
	var extraWork = len(*hashes) % hashWorkers
	var lastStart = 0
	for i := 0; i < hashWorkers; i++ {
		var end = lastStart + divWork
		if i < extraWork {
			end++
		}
		go func(trees []*BstNode, start int, end int) {
			for i := start; i < end; i++ {
				(*hashes)[i] = trees[i].genHash()
			}
			wg.Done()
		}((*trees), lastStart, end)
		lastStart = end
	}
	wg.Wait()
}

func computeHashesAndGroup(trees *[]*BstNode, hashesMap *map[int][]int, hashWorkers int, dataWorkers int) time.Duration {
	if hashWorkers == 1 {
		// fmt.Println("SeqHash")
		return computeHashesAndGroupSeq(trees, hashesMap)
	} else if dataWorkers == 1 {
		// fmt.Println("channel Hash GRoup")
		return computeHashesAndGroupChannel(trees, hashesMap, hashWorkers)
	} else {
		// fmt.Println("Mutex Hash Group")
		return computeHashesAndGroupMutex(trees, hashesMap, hashWorkers)
	}
	// optional implementation doesn't exist (yet?)
}

func computeHashesAndGroupSeq(trees *[]*BstNode, hashesMap *map[int][]int) time.Duration {
	startTime := time.Now()
	hashes := make([]int, len(*trees))
	for i := range hashes {
		hashes[i] = (*trees)[i].genHash()
	}
	hashTime := time.Since(startTime)
	for i := range hashes {
		(*hashesMap)[hashes[i]] = append((*hashesMap)[hashes[i]], i)
	}
	return hashTime
}

type Pair struct {
	a int
	b int
}

func computeHashesAndGroupChannel(trees *[]*BstNode, hashesMap *map[int][]int, hashWorkers int) time.Duration {
	startTime := time.Now()
	myChannel := make(chan Pair, 5)
	var wg sync.WaitGroup
	wg.Add(hashWorkers)
	var divWork = len(*trees) / hashWorkers
	var extraWork = len(*trees) % hashWorkers
	var lastStart = 0
	for i := 0; i < hashWorkers; i++ {
		var end = lastStart + divWork
		if i < extraWork {
			end++
		}
		go func(trees []*BstNode, start int, end int) {
			for i := start; i < end; i++ {
				var hash = trees[i].genHash()
				myChannel <- Pair{hash, i}
			}
			wg.Done()
		}((*trees), lastStart, end)
		lastStart = end
	}
	for i := 0; i < len(*trees); i++ {
		var temp = <-myChannel
		(*hashesMap)[temp.a] = append((*hashesMap)[temp.a], temp.b)
	}
	wg.Wait()
	return time.Since(startTime)
}

func computeHashesAndGroupMutex(trees *[]*BstNode, hashesMap *map[int][]int, hashWorkers int) time.Duration {
	startTime := time.Now()
	var myLock sync.RWMutex
	var wg sync.WaitGroup
	wg.Add(hashWorkers)
	var divWork = len(*trees) / hashWorkers
	var extraWork = len(*trees) % hashWorkers
	var lastStart = 0
	for i := 0; i < hashWorkers; i++ {
		var end = lastStart + divWork
		if i < extraWork {
			end++
		}
		go func(hashesMap *map[int][]int, trees []*BstNode, start int, end int) {
			for i := start; i < end; i++ {
				var hash = trees[i].genHash()
				myLock.Lock()
				(*hashesMap)[hash] = append((*hashesMap)[hash], i)
				myLock.Unlock()
			}
			wg.Done()
		}(*&hashesMap, (*trees), lastStart, end)
		lastStart = end
	}
	wg.Wait()
	return time.Since(startTime)
}

func compareTrees(trees *[]*BstNode, hashesMap *map[int][]int, result *[][]int, keys *[]int, workers int) {
	if workers == 1 {
		compareTreesSeq(trees, hashesMap, result, keys)
	} else {
		compareTreesParallelWorkers(trees, hashesMap, result, keys, workers)
	}
}

func compareTreesSeq(trees *[]*BstNode, hashesMap *map[int][]int, result *[][]int, keys *[]int) {
	equivs := make([][]int, len(*trees))
	for i := range equivs {
		equivs[i] = []int{}
	}
	for _, key := range *keys {
		for i := range (*hashesMap)[key] {
			var length = len((*hashesMap)[key])
			// var resInd = len(groups)
			for j := i + 1; j < length; j++ {
				var treeA = (*hashesMap)[key][i]
				var treeB = (*hashesMap)[key][j]
				var res = (*trees)[treeA].compareTo((*trees)[treeB])
				if res {
					equivs[treeA] = append(equivs[treeA], treeB)
					equivs[treeB] = append(equivs[treeB], treeA)
				}
			}
		}
	}
	tempMap := make(map[int]bool)
	for i := range equivs {
		var resInd = len(*result)

		for j := range equivs[i] {
			if !tempMap[equivs[i][j]] && len(equivs[i]) > 0 {
				tempMap[i] = true
				if resInd == len(*result) {
					(*result) = append((*result), []int{})
					(*result)[resInd] = append((*result)[resInd], i)
				}
				(*result)[resInd] = append((*result)[resInd], equivs[i][j])
				tempMap[equivs[i][j]] = true
			}
		}
	}
}

func compareTreesParallel(trees *[]*BstNode, hashesMap *map[int][]int, result *[][]int, keys *[]int) {
	equivs := make([][]int, len(*trees))
	for i := range equivs {
		equivs[i] = []int{}
	}
	var wg sync.WaitGroup
	var myLock sync.RWMutex

	for _, key := range *keys {
		for i := range (*hashesMap)[key] {
			var length = len((*hashesMap)[key])
			// var resInd = len(groups)
			for j := i + 1; j < length; j++ {
				var treeAIndex = (*hashesMap)[key][i]
				var treeBIndex = (*hashesMap)[key][j]
				wg.Add(1)
				go func(tree1 *BstNode, tree2 *BstNode, equivs *[][]int, i int, j int) {
					var res = tree1.compareTo(tree2)
					if res {
						myLock.Lock()
						(*equivs)[i] = append((*equivs)[i], j)
						(*equivs)[j] = append((*equivs)[j], i)
						myLock.Unlock()
					}
					wg.Done()
				}((*trees)[treeAIndex], (*trees)[treeBIndex], &equivs, treeAIndex, treeBIndex)
			}
		}
	}

	wg.Wait()
	tempMap := make(map[int]bool)
	for i := range equivs {
		var resInd = len(*result)

		for j := range equivs[i] {
			if !tempMap[equivs[i][j]] && len(equivs[i]) > 0 {
				tempMap[i] = true
				if resInd == len(*result) {
					(*result) = append((*result), []int{})
					(*result)[resInd] = append((*result)[resInd], i)
				}
				(*result)[resInd] = append((*result)[resInd], equivs[i][j])
				tempMap[equivs[i][j]] = true
			}
		}
	}
}

type ConcurrentBuffer struct {
	sync.RWMutex
	work     []Pair
	notFull  sync.Cond
	notEmpty sync.Cond
	maxSize  int
	// completed int
	// expected  int
}

func createBuffer(buffSize int) *ConcurrentBuffer {
	buff := ConcurrentBuffer{}
	buff.work = []Pair{}
	buff.notFull = *sync.NewCond(&buff)
	buff.notEmpty = *sync.NewCond(&buff)
	buff.maxSize = buffSize
	// buff.expected = 0
	// buff.completed = 0
	return &buff
}

func (buff *ConcurrentBuffer) pop() Pair {
	buff.Lock()
	defer buff.Unlock()
	for len(buff.work) == 0 {
		buff.notEmpty.Wait()
	}
	returnVal := buff.work[0]
	buff.work = buff.work[1:len(buff.work)] // take front out of queue
	buff.notFull.Signal()

	return returnVal
}

func (buff *ConcurrentBuffer) push(item Pair) {
	buff.Lock()
	defer buff.Unlock()
	for len(buff.work) == buff.maxSize {
		buff.notFull.Wait()
	}
	buff.work = append(buff.work, item)
	// buff.expected++
	buff.notEmpty.Signal()
	// buff.notFull.Signal()
}

func (buff *ConcurrentBuffer) pushFake() {
	buff.Lock()
	defer buff.Unlock()
	for len(buff.work) == buff.maxSize {
		buff.notFull.Wait()
	}

	buff.work = append(buff.work, Pair{-1, -1})
	buff.notEmpty.Signal()
}

func compareTreesParallelWorkers(trees *[]*BstNode, hashesMap *map[int][]int, result *[][]int, keys *[]int, workers int) {
	buffer := createBuffer(workers)

	equivs := make([][]int, len(*trees))
	for i := range equivs {
		equivs[i] = []int{}
	}
	var wg sync.WaitGroup
	// var myLock sync.RWMutex
	wg.Add(workers)

	for i := 0; i < workers; i++ {
		go func(trees *[]*BstNode, equivs *[][]int) {
			defer wg.Done()
			for {
				var currPair = Pair{}

				currPair = buffer.pop()
				var treeAIndex = currPair.a
				if treeAIndex == -1 {
					break
				}
				var treeBIndex = currPair.b
				var res = (*trees)[treeAIndex].compareTo((*trees)[treeBIndex])
				if res {
					buffer.Lock()
					(*equivs)[treeAIndex] = append((*equivs)[treeAIndex], treeBIndex)
					(*equivs)[treeBIndex] = append((*equivs)[treeBIndex], treeAIndex)
					buffer.Unlock()
				}
				// buffer.Lock()
				// buffer.completed++
				// buffer.Unlock()
			}
		}(trees, &equivs)
	}
	go func(trees *[]*BstNode, hashesMap *map[int][]int, equivs *[][]int) {
		var jobs = 0
		for _, key := range *keys {
			for i := range (*hashesMap)[key] {
				var length = len((*hashesMap)[key])
				// var resInd = len(groups)
				for j := i + 1; j < length; j++ {
					var treeAIndex = (*hashesMap)[key][i]
					var treeBIndex = (*hashesMap)[key][j]
					buffer.push(Pair{treeAIndex, treeBIndex})
					jobs++
				}
			}
		}
		for i := 0; i < workers; i++ {
			buffer.pushFake()
		}
	}(trees, hashesMap, &equivs)
	wg.Wait()
	tempMap := make(map[int]bool)
	for i := range equivs {
		var resInd = len(*result)

		for j := range equivs[i] {
			if !tempMap[equivs[i][j]] && len(equivs[i]) > 0 {
				tempMap[i] = true
				if resInd == len(*result) {
					(*result) = append((*result), []int{})
					(*result)[resInd] = append((*result)[resInd], i)
				}
				(*result)[resInd] = append((*result)[resInd], equivs[i][j])
				tempMap[equivs[i][j]] = true
			}
		}
	}
}
