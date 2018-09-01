import sys
import io
from collections import defaultdict

################################################################################

LINE_LEN = 80

################################################################################

class Dictionary:
    def __init__(self):
        # key = word length, value = list of key-length words in the dictionary
        self.wordsDict = {}
        # list of word-lengths stored wordDict, sorted
        self.wordLengths = []
        # wordAmounts[n] tells the number of wordLengths[n]-length words in
        # wordsDict
        self.wordAmounts = []

    def readWords(self,textfile):
        self.wordsDict = defaultdict(list)
        with io.open(textfile, encoding='utf-8') as infile:
            for line in infile:
                for word in line.split():
                    word += ' ' # Pad with space
                    key = len(word)
                    if key > LINE_LEN:
                        msg = ("ERROR: textfile contains words longer than the"
                               "specified line length: %s")
                        print(msg % (LINE_LEN))
                        sys.exit(0)
                    self.wordsDict[key].append(word)
        for key in sorted(self.wordsDict):
            self.wordLengths.append(key)
            self.wordAmounts.append(len(self.wordsDict[key]))

    def popWord(self, wordsize):
        word = self.wordsDict[wordsize].pop()
        if not self.wordsDict[wordsize]:
            del self.wordsDict[wordsize]
        pos = self.wordLengths.index(wordsize)
        self.wordAmounts[pos] -= 1
        if self.wordAmounts[pos] == 0:
            del self.wordAmounts[pos]
            del self.wordLengths[pos]
        return word

    # Return True if all words are used
    def allWordsUsed(self):
        return bool(self.wordLengths) == False

################################################################################

class Memory:
    def __init__(self):
        # cache earlier results to prevent having to calculate same
        # sequences over again
        self.cache = {}

    def store(self, stamp, data):
        self.cache[stamp] = data

################################################################################

class Sequencer:
    def __init__(self, keys, supply, memory):
        # list of keys, sorted
        self.keys = keys
        # supply of each key (supply[n] is the supply of key[n]-keys)
        self.supply = supply
        # memory to store the earlier results that can be pruned
        self.memory = memory

    def canPrune(self, stamp):
        # We can prune the sequence if target and keys match (stamp)
        # and supply has not increased since the stamp was cached.
        cached_supply = self.memory.cache.get(stamp, "NA")
        if cached_supply == 'NA':
            return False
        for idx, val in enumerate(cached_supply):
            if self.supply[idx] > val:
                return False
        return True

    def replicate(self):
        # keys and supply need to be own instances for each object, memory
        # is shared between parents and child's
        new = Sequencer(self.keys[:], self.supply[:], self.memory)
        return new

    def selectOne(self, len):
        # Take one len-length key: reduce supply and remove from keys
        # if it was the last one
        pos = self.keys.index(len)
        self.supply[pos] -= 1
        if not self.supply[pos]:
            del self.supply[pos]
            del self.keys[pos]

    def findSequence(self, target):
        # Recursively find a sequence of keys that sum to target.
        # Returns the sequence of keys in an array.
        if target <= 1:
            return None
        # Do we know this sequence will not complete?
        stamp = str(target)+str(self.keys)
        skip = self.canPrune(stamp)
        if skip:
            return None
        # Recursively try all sequences starting from the longest keys
        for len in reversed(self.keys):
            if len > target:
                continue
            if len == target:
                # Found leaf
                return [len]
            new = self.replicate()
            new.selectOne(len)
            sequence = new.findSequence(target-len)
            if sequence:
                # len is a node in the sequence
                return [len] + sequence

        # store if no sequence found
        self.memory.store(stamp, self.supply)

################################################################################

def shorten(dict):
     # LINE_LEN + 1 to allow trailing space.
     # The trailing space will be removed before printing the line
    targetLineLength = LINE_LEN + 1
    memory = Memory()

    while not dict.allWordsUsed():
        # Build one line: find a sequence of word-lengths that sum
        # to targetLineLength
        sequencer = Sequencer(dict.wordLengths, dict.wordAmounts, memory)
        seq = sequencer.findSequence(targetLineLength)
        if not seq:
            # No sequence found using current targetLineLength.
            # If possible, reduce the targetLineLength and try again.
            if targetLineLength > 0:
                targetLineLength -= 1
                continue
            else:
                raise Exception("No sequence found")

        # Sequence of word-lengths found. Now pop the actual words from
        # the dictionary.
        words = []
        for wordlen in seq:
            word = dict.popWord(wordlen)
            words.append(word)

        # Each word includes a trailing space, therefore, no need to join
        # the words with spaces...
        line = ''.join(words)
        # ... but the last trailing space needs to be removed
        line = line[:-1]
        print line

################################################################################

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "Usage: %s <textfile>" % __file__
        sys.exit(0)

    reload(sys)
    sys.setdefaultencoding('utf8')
    infile = sys.argv[1]
    dict = Dictionary()
    dict.readWords(infile)
    shorten(dict)

################################################################################
