#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

////////////////////////////////////////////////////////////////////////////////

// Target "shortened" line length.
#define TARGET_LINE_LEN 80

// Max wordlength: words longer than this will be split
#define MAX_WORD_LEN 100

// https://stackoverflow.com/questions/47346133
#define STR_(X) #X
#define STR(X) STR_(X)

typedef unsigned char sint;

////////////////////////////////////////////////////////////////////////////////

typedef struct ListEntry {
    wchar_t* data;
    struct ListEntry* next;

} ListEntry;

ListEntry* createEntry(ListEntry* next, wchar_t* data) {
    ListEntry* new = malloc(sizeof(ListEntry));
    if (new == NULL) {
        printf("ERROR: memory allocation failed\n");
        exit(1);
    }
    new->data = data;
    new->next = next;
    return new;
}

ListEntry* popEntry(ListEntry** head) {
    if(head == NULL) {
        return NULL;
    }
    ListEntry* temp = *head;
    if(temp) {
        *head = temp->next;
        temp->next = NULL;
    }
    return temp;
}

////////////////////////////////////////////////////////////////////////////////

// We use MAX_WORD_LEN_IDX when reserving arrays to allow indexing with word
// lengths directly.
#define MAX_WORD_LEN_IDX (TARGET_LINE_LEN+2)

typedef struct Dictionary {
    // wordsDict[n] stores a list of n-length words (space appended).
    // For instance, if input is a file containing words 'aa', 'aaaa', 'bbbb',
    // then
    // wordsDict[3] --> "aa "
    // wordsDict[5] --> "aaaa " --> "bbbb "
    ListEntry* wordsDict[MAX_WORD_LEN_IDX];
    sint wordsDictElements;
    sint longestWord;

    // wordAmounts[n] tells the number of n-length words (space appended) in
    // wordsDict.
    // For instance, if input is a file containing words 'aa', 'aaaa', 'bbbb',
    // then
    // wordAmounts[3] = 1
    // wordAmounts[5] = 2
    unsigned wordAmounts[MAX_WORD_LEN_IDX];
} Dictionary;

Dictionary* createDictionary(char* textfile) {
    FILE *fp = fopen(textfile,"r");
    if(!fp) {
        printf("ERROR: failed to open file: \"%s\"\n", textfile);
        exit(1);
    }
    Dictionary* dict = calloc(1,sizeof(Dictionary));
    if(dict == NULL) {
        printf("ERROR: memory allocation failed\n");
        exit(1);
    }
    // Words longer than MAX_WORD_LEN will be split
    wchar_t str[MAX_WORD_LEN+1];
    wchar_t* word = NULL;
    sint wordlen = 0;
    while (fwscanf(fp, L" %"STR(MAX_WORD_LEN)"ls", str) == 1) {
        // +1 since we are going to append a space character
        wordlen = wcslen(str) + 1;
        if(wordlen-1 > TARGET_LINE_LEN) {
            printf("ERROR: textfile contains words longer than the specified "
                   "line length: %d\n", TARGET_LINE_LEN);
            exit(1);
        }
        word = malloc((sizeof(wchar_t)*(wordlen+1)));
        if(word == NULL) {
            printf("ERROR: memory allocation failed\n");
            exit(1);
        }

        wcscpy(word, str);
        wcscat(word, L" ");
        dict->wordsDict[wordlen] = createEntry(dict->wordsDict[wordlen], word);
        dict->wordAmounts[wordlen]++;
    }
    fclose(fp);
    for(sint i = 0; i < MAX_WORD_LEN_IDX; i++) {
        if(dict->wordsDict[i] != NULL) {
            dict->wordsDictElements++;
            dict->longestWord = i;
        }
    }
    return dict;
}

wchar_t* popWord(Dictionary* this, sint wordlen) {
    if(wordlen >= MAX_WORD_LEN_IDX || this->wordsDict[wordlen] == NULL) {
        return NULL;
    }
    ListEntry* popped = popEntry(&(this->wordsDict[wordlen]));
    wchar_t* word = NULL;
    if(popped) {
        this->wordAmounts[wordlen]--;
        if(this->wordAmounts[wordlen] == 0) {
            this->wordsDictElements--;
        }
        word = popped->data;
        free(popped);
    }
    return word;
}

bool allWordsUsed(Dictionary* this) {
    return (this->wordsDictElements == 0);
}

////////////////////////////////////////////////////////////////////////////////

// Words are separated with spaces and shortest word is 1. Knowing this,
// MAX_SEQ_LEN estimates the upper limit for the sequence (number of words)
// on a line.
#define MAX_SEQ_LEN ((sint) (MAX_WORD_LEN_IDX) / (2))

typedef struct Sequencer {
    // Shared between instances
    Dictionary* dict;
    sint* sequence;
    sint* seqNextIdx;

    // Own copy for each instance
    unsigned wordAmounts[MAX_WORD_LEN_IDX];
} Sequencer;

Sequencer* createSequencer(Dictionary* dict) {
    Sequencer* s = calloc(1,sizeof(Sequencer));
    if(s == NULL) {
        printf("ERROR: memory allocation failed\n");
        exit(1);
    }
    s->dict = dict;
    memcpy(s->wordAmounts, dict->wordAmounts, sizeof(s->wordAmounts));
    s->sequence = calloc(1, MAX_SEQ_LEN * sizeof(sint));
    if(s->sequence == NULL) {
        printf("ERROR: memory allocation failed\n");
        exit(1);
    }
    s->seqNextIdx = calloc(1,sizeof(sint));
    if(s->seqNextIdx == NULL) {
        printf("ERROR: memory allocation failed\n");
        exit(1);
    }
    return s;
}

void deallocateSequencer(Sequencer* sequencer) {
    free(sequencer->sequence);
    free(sequencer->seqNextIdx);
    free(sequencer);
}

Sequencer* replicateSequencer(Sequencer* this) {
    Sequencer* new = malloc(sizeof(Sequencer));
    if(new == NULL) {
        printf("ERROR: memory allocation failed\n");
        exit(1);
    }
    memcpy(new, this, sizeof(Sequencer));
    return new;
}

bool findSequence(Sequencer* this, sint target) {
    if(target <= 1) {
        return false;
    }
    Sequencer* new;
    for(int len = this->dict->longestWord; len >= 0; len--) {
        if(this->wordAmounts[len] == 0) {
            continue;
        }
        if(len > target) {
            continue;
        }
        if(len == target) {
            this->sequence[*this->seqNextIdx] = len;
            (*this->seqNextIdx)++;
            return true;
        }
        new = replicateSequencer(this);
        new->wordAmounts[len]--;
        if(findSequence(new, target-len)) {
            new->sequence[*new->seqNextIdx] = len;
            (*new->seqNextIdx)++;
            free(new);
            return true;
        }
        free(new);
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

void shorten(Dictionary* dict) {
    sint targetLineLength = TARGET_LINE_LEN + 1;
    Sequencer* sequencer;
    while(!allWordsUsed(dict)) {
        sequencer = createSequencer(dict);
        if(!findSequence(sequencer, targetLineLength)) {
            if(targetLineLength > 0) {
                targetLineLength--;
                deallocateSequencer(sequencer);
                continue;
            }
            else {
                printf("ERROR: no sequence found\n");
                exit(1);
            }
        }
        wchar_t* word;
        sint wordlen;
        for(sint i = 0; i < *sequencer->seqNextIdx; i++) {
            wordlen = sequencer->sequence[i];
            word = popWord(dict, wordlen);
            // Remove trailing whitespace from the last word of the line
            if(i == (*sequencer->seqNextIdx)-1 ) {
                word[wordlen-1] = '\0';
            }
            printf("%ls", word);
            free(word);
        }
        printf("\n");
        deallocateSequencer(sequencer);
    }
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    if(argc < 2) {
        printf("Usage: %s <textfile>\n", argv[0]);
        return 0;
    }

    if(TARGET_LINE_LEN > MAX_WORD_LEN) {
        printf("ERROR: specified line length is longer than MAX_WORD_LEN\n");
        exit(1);
    }

    char *textfile = argv[1];
    Dictionary* dict = createDictionary(textfile);
    shorten(dict);
    free(dict);
}

////////////////////////////////////////////////////////////////////////////////
