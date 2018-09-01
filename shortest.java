import java.lang.*;
import java.util.*;
import java.io.*;

////////////////////////////////////////////////////////////////////////////////

class Globals {
    public static int TARGET_LINE_LEN = 80;
}

////////////////////////////////////////////////////////////////////////////////

// https://stackoverflow.com/questions/8559092
class ListOfStrings extends LinkedList<String> { }

class Dictionary {
    // We use MAX_WORD_LEN_IDX when reserving arrays to allow indexing with 
    // word lengths directly.
    public static int MAX_WORD_LEN_IDX = Globals.TARGET_LINE_LEN + 2;

    // wordsDict[n] stores a list of n-length words (space appended).
    // For instance, if input is a file containing words 'aa', 'aaaa', 'bbbb',
    // then
    // wordsDict[3] --> "aa "
    // wordsDict[5] --> "aaaa " --> "bbbb " 
    ListOfStrings[] wordsDict = new ListOfStrings[MAX_WORD_LEN_IDX];
    int wordsDictElements = 0;
    int longestWord = 0;

    // wordAmounts[n] tells the number of n-length words (space appended) in
    // wordsDict.
    // For instance, if input is a file containing words 'aa', 'aaaa', 'bbbb',
    // then
    // wordAmounts[3] = 1
    // wordAmounts[5] = 2
    int[] wordAmounts = new int[MAX_WORD_LEN_IDX];

    Dictionary(String textfile) throws IOException {
        File infile = new File(textfile);
        Scanner input = new Scanner(infile);
        while(input.hasNext()) {
            String word = input.next() + " ";
            int wordlen = word.length();
            if(wordlen-1 > Globals.TARGET_LINE_LEN) {
                System.out.printf("ERROR: textfile contains words longer than " +
                    "the specified line length: %d\n", Globals.TARGET_LINE_LEN);
                System.exit(1);
            }
            ListOfStrings listOfwordlenWords = this.wordsDict[wordlen];
            if(listOfwordlenWords == null) {
                this.wordsDict[wordlen] = new ListOfStrings();
            }
            this.wordsDict[wordlen].add(word);
            this.wordAmounts[wordlen]++;
        }
        input.close();
        for(int i = 0; i < MAX_WORD_LEN_IDX; i++) {
            if(this.wordsDict[i] != null) {
                this.wordsDictElements++;
                this.longestWord = i;
            }
        }
    }

    String popWord(int wordlen) {
        if( wordlen >= MAX_WORD_LEN_IDX || 
            this.wordsDict[wordlen] == null || 
            this.wordsDict[wordlen].isEmpty() ) {
            return null;
        }
        String word = this.wordsDict[wordlen].pop();
        if(word != null) {
            this.wordAmounts[wordlen]--;
            if(this.wordAmounts[wordlen] == 0) {
                this.wordsDictElements--;
            }
        }
        return word;
    }

    boolean allWordsUsed() {
        return (this.wordsDictElements == 0);
    }

    int[] getWordAmounts() {
        return this.wordAmounts;
    }

    int getLongestWord() {
        return this.longestWord;
    }
}

////////////////////////////////////////////////////////////////////////////////

class ListOfInts extends LinkedList<Integer> { }

class Sequencer {
    // Shared between instances
    Dictionary dict;
    ListOfInts sequence;

    // Own copy for each instance
    int[] wordAmounts = new int[Dictionary.MAX_WORD_LEN_IDX];

    Sequencer(Dictionary dict, int[] wordamounts, ListOfInts sequence) {
        this.dict = dict;
        if(sequence != null) {
            this.sequence = sequence;
        }
        else {
            this.sequence = new ListOfInts();
        }
        this.wordAmounts = Arrays.copyOf(wordamounts, 
            Dictionary.MAX_WORD_LEN_IDX);
    }

    Sequencer replicate() {
        Sequencer newSequencer = new Sequencer(
            this.dict, this.wordAmounts, this.sequence);
        return newSequencer;
    }

    boolean findSequence(int target) {
        if(target <= 1){
            return false;
        }

        for(int len = this.dict.getLongestWord(); len >= 0; len-- ) {
            if(this.wordAmounts[len] == 0) {
                continue;
            }
            if(len > target) {
                continue;
            }
            if(len == target) {
                // Found leaf
                this.sequence.add(len);
                return true;
            }
            Sequencer copyOfThis = this.replicate();
            copyOfThis.wordAmounts[len]--;
            if(copyOfThis.findSequence(target - len)) {
                // len is a node on a sequence to leaf
                copyOfThis.sequence.add(len);
                return true;
            }
        }
        return false;
    }

    ListOfInts getSequence() {
        return this.sequence;
    }
}

////////////////////////////////////////////////////////////////////////////////

class Shortener {
    static void shorten(Dictionary dict) {
        int targetLineLength = Globals.TARGET_LINE_LEN + 1;
        Sequencer sequencer;
        while(!dict.allWordsUsed()) {
            sequencer = new Sequencer(dict, dict.getWordAmounts(), null);
            if(!sequencer.findSequence(targetLineLength)) {
                if(targetLineLength > 0) {
                    // No sequence found using current targetLineLength.
                    // If possible, reduce the targetLineLenght and try again.
                    targetLineLength--;
                    continue;
                }
                else {
                    System.out.println("ERROR: no sequence found");
                    System.exit(1);
                }
            }
            String line = "";
            ListOfInts seq = sequencer.getSequence();
            for(int wordlen : seq) {
                String word = dict.popWord(wordlen);
                line += word;
            }
            line = line.trim();
            System.out.println(line);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

public class shortest {
    public static void main(String[] args) throws IOException {
        if(args.length < 1) {
            String myname = shortest.class.getSimpleName();
            System.out.printf("Usage: java %s <textfile>\n", myname);
            System.exit(1);
        }
        String textfile = args[0];
        Dictionary dict = new Dictionary(textfile);
        Shortener.shorten(dict);
    }
}

////////////////////////////////////////////////////////////////////////////////