#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Node structure for Huffman Tree */
typedef struct Node {
    unsigned char ch;
    unsigned char minCh; /* smallest character in this subtree (tie-breaker) */
    int freq;
    struct Node *left, *right;
} Node;

Node *createLeaf(unsigned char ch, int freq) {
    Node *n = (Node *)malloc(sizeof(Node));
    n->ch = ch;
    n->minCh = ch;
    n->freq = freq;
    n->left = n->right = NULL;
    return n;
}

Node *createParent(Node *l, Node *r) {
    Node *n = (Node *)malloc(sizeof(Node));
    n->ch = 0;
    n->minCh = (l->minCh < r->minCh) ? l->minCh : r->minCh;
    n->freq = l->freq + r->freq;
    n->left = l;
    n->right = r;
    return n;
}

void freeTree(Node *root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

/* Min-Heap (priority queue) stored in an array */
typedef struct {
    Node *data[256];
    int size;
} MinHeap;

/* Comparison: smaller frequency first, ties broken by smallest character */
int isLess(Node *a, Node *b) {
    if (a->freq != b->freq) return a->freq < b->freq;
    return a->minCh < b->minCh;
}

void heapPush(MinHeap *h, Node *n) {
    int i = h->size++;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (!isLess(n, h->data[parent])) break;
        h->data[i] = h->data[parent];
        i = parent;
    }
    h->data[i] = n;
}

Node *heapPop(MinHeap *h) {
    Node *top = h->data[0];
    Node *last = h->data[--h->size];
    int i = 0;
    while (2 * i + 1 < h->size) {
        int c = 2 * i + 1;
        if (c + 1 < h->size && isLess(h->data[c + 1], h->data[c])) c++;
        if (!isLess(h->data[c], last)) break;
        h->data[i] = h->data[c];
        i = c;
    }
    if (h->size > 0) h->data[i] = last;
    return top;
}

/* BitWriter: packs individual bits into 8-bit bytes */
typedef struct {
    FILE *out;
    unsigned char buffer;
    int bitCount;
} BitWriter;

void writeBit(BitWriter *w, int bit) {
    w->buffer = (unsigned char)((w->buffer << 1) | (bit & 1));
    w->bitCount++;
    if (w->bitCount == 8) {
        fputc(w->buffer, w->out);
        w->buffer = 0;
        w->bitCount = 0;
    }
}

void writeBits(BitWriter *w, const char *bitStr) {
    for (int i = 0; bitStr[i] != '\0'; i++)
        writeBit(w, bitStr[i] == '1' ? 1 : 0);
}

void flushBits(BitWriter *w) {
    if (w->bitCount > 0) {
        w->buffer = (unsigned char)(w->buffer << (8 - w->bitCount));
        fputc(w->buffer, w->out);
        w->buffer = 0;
        w->bitCount = 0;
    }
}

/* BitReader: extracts bits byte by byte */
typedef struct {
    FILE *in;
    unsigned char buffer;
    int bitCount;
} BitReader;

int readBit(BitReader *r) {
    if (r->bitCount == 0) {
        int c = fgetc(r->in);
        if (c == EOF) return -1;
        r->buffer = (unsigned char)c;
        r->bitCount = 8;
    }
    r->bitCount--;
    return (r->buffer >> r->bitCount) & 1;
}

/* Builds Huffman tree from frequency table */
Node *buildTree(const int freq[256]) {
    MinHeap heap;
    heap.size = 0;
    for (int i = 0; i < 256; i++)
        if (freq[i] > 0) heapPush(&heap, createLeaf((unsigned char)i, freq[i]));
    if (heap.size == 0) return NULL;
    while (heap.size > 1) {
        Node *left = heapPop(&heap);
        Node *right = heapPop(&heap);
        heapPush(&heap, createParent(left, right));
    }
    return heapPop(&heap);
}

/* Traverses tree to build codebook */
void generateCodes(Node *root, char *buf, int depth, char *codes[256]) {
    if (!root) return;
    if (!root->left && !root->right) {
        buf[depth] = '\0';
        const char *code = (depth == 0) ? "0" : buf;
        codes[root->ch] = (char *)malloc(strlen(code) + 1);
        strcpy(codes[root->ch], code);
        return;
    }
    buf[depth] = '0';
    generateCodes(root->left, buf, depth + 1, codes);
    buf[depth] = '1';
    generateCodes(root->right, buf, depth + 1, codes);
}

/* Compress Input File */
void compress(const char *inputFile, const char *outputFile) {
    FILE *in = fopen(inputFile, "rb");
    if (!in) {
        fprintf(stderr, "Error: Could not open input file.\n");
        return;
    }

    int freq[256] = {0};
    long long totalChars = 0;
    int c;
    while ((c = fgetc(in)) != EOF) {
        freq[c]++;
        totalChars++;
    }

    if (totalChars == 0) {
        fprintf(stderr, "Error: Input file is empty.\n");
        fclose(in);
        return;
    }

    Node *root = buildTree(freq);
    char *codes[256] = {0};
    char buf[257];
    generateCodes(root, buf, 0, codes);

    FILE *out = fopen(outputFile, "wb");
    if (!out) {
        fprintf(stderr, "Error: Could not create output file.\n");
        fclose(in);
        for (int i = 0; i < 256; i++) free(codes[i]);
        freeTree(root);
        return;
    }

    /* Header serialization: unique character count and total characters */
    int uniqueCount = 0;
    for (int i = 0; i < 256; i++) if (freq[i] > 0) uniqueCount++;
    fwrite(&uniqueCount, sizeof(uniqueCount), 1, out);
    fwrite(&totalChars, sizeof(totalChars), 1, out);

    /* Write frequency table to header */
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            fputc(i, out);
            fwrite(&freq[i], sizeof(int), 1, out);
        }
    }

    /* Write encoded payload */
    BitWriter writer = {out, 0, 0};
    rewind(in);
    while ((c = fgetc(in)) != EOF) {
        writeBits(&writer, codes[c]);
    }
    flushBits(&writer);

    fclose(in);
    fclose(out);
    for (int i = 0; i < 256; i++) free(codes[i]);
    freeTree(root);
    printf("Compression successful: %s\n", outputFile);
}

/* Decompress File */
void decompress(const char *inputFile, const char *outputFile) {
    FILE *in = fopen(inputFile, "rb");
    if (!in) {
        fprintf(stderr, "Error: Could not open compressed file.\n");
        return;
    }

    /* Read header details */
    int uniqueCount = 0;
    long long totalChars = 0;
    if (fread(&uniqueCount, sizeof(uniqueCount), 1, in) != 1 ||
        fread(&totalChars, sizeof(totalChars), 1, in) != 1) {
        fprintf(stderr, "Error: Invalid compressed file.\n");
        fclose(in);
        return;
    }

    int freq[256] = {0};
    for (int i = 0; i < uniqueCount; i++) {
        int ch = fgetc(in);
        int f;
        if (ch == EOF || fread(&f, sizeof(f), 1, in) != 1) {
            fprintf(stderr, "Error: Invalid compressed file.\n");
            fclose(in);
            return;
        }
        freq[ch] = f;
    }

    Node *root = buildTree(freq);
    FILE *out = fopen(outputFile, "wb");
    if (!out || !root) {
        fprintf(stderr, "Error: Could not create output file.\n");
        if (out) fclose(out);
        fclose(in);
        freeTree(root);
        return;
    }

    long long decodedChars = 0;
    if (!root->left && !root->right) {
        while (decodedChars++ < totalChars) fputc(root->ch, out);
    } else {
        BitReader reader = {in, 0, 0};
        Node *current = root;
        while (decodedChars < totalChars) {
            int bit = readBit(&reader);
            if (bit == -1) break;
            current = (bit == 0) ? current->left : current->right;
            if (!current->left && !current->right) {
                fputc(current->ch, out);
                decodedChars++;
                current = root;
            }
        }
    }

    fclose(in);
    fclose(out);
    freeTree(root);
    printf("Decompression successful: %s\n", outputFile);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage:\n");
        printf("  Compress:   %s -c <input_file> <output_file>\n", argv[0]);
        printf("  Decompress: %s -d <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    const char *mode = argv[1];
    const char *inputFile = argv[2];
    const char *outputFile = argv[3];

    if (strcmp(mode, "-c") == 0) {
        compress(inputFile, outputFile);
    } else if (strcmp(mode, "-d") == 0) {
        decompress(inputFile, outputFile);
    } else {
        fprintf(stderr, "Invalid flag. Use -c for compression or -d for decompression.\n");
        return 1;
    }

    return 0;
}
