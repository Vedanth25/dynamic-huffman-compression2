# Dynamic Huffman File Compression Tool

A command-line utility written in C (C99) for lossless text file compression and decompression using **Huffman Coding**, **Min-Heap Priority Queues**, and **Binary Prefix Trees**.

Developed as part of the **Data Structures (25CSE201)** course at **Nitte Meenakshi Institute of Technology (NMIT), Bengaluru** (2026-2027).

---

## Team Members
* **Vedanth S** - `NB25ISE263`
* **Yuvan S M Dixith** - `NB25ISE276`
* **Varun Veda** - `NB25ISE262`

**Faculty Guide:** Mrs. M. Lakshmi, Assistant Professor, Dept. of ISE, NMIT

---

## Features & Data Structures Used
* **Frequency Table (Array):** Counts byte occurrences in $O(N)$ time using a 256-element integer array.
* **Priority Queue (Array-based Min-Heap):** Repeatedly extracts the two lowest-frequency nodes in $O(\log k)$ time to construct the optimal tree.
* **Huffman Tree (Binary Prefix Tree):** Generates variable-length prefix codes (`0` for left, `1` for right) ensuring unambiguous decoding.
* **Bit-Level I/O (`BitWriter` & `BitReader`):** Packs variable-length binary codes into 8-bit bytes and serializes the frequency table header into a `.huff` file.

---

## Project Structure
```text
├── huffman.c      # Complete C source code (Min-Heap, Tree, BitWriter/BitReader, CLI)
├── sample.txt     # Sample input text file for testing
└── README.md      # Project documentation
```

---

## Compilation & Usage

### 1. Compile the Program
```bash
gcc -std=c99 -O2 huffman.c -o huffman
```

### 2. Compress a File (`-c`)
```bash
./huffman -c sample.txt compressed.huff
```

### 3. Decompress a File (`-d`)
```bash
./huffman -d compressed.huff restored.txt
```
