// filehdr.h
//	Data structures for managing a disk file header.
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "bitmap.h"

// ****lab5 code begin****
#define DataForm 25 //length of time info in filehdr

//secondary index
#define SecondIndex 5 //define secondary index maximum num
#define SecondDirect SectorSize / sizeof(int) //secondary index pointed maximum file space

//direct index
#define NumDirect 	((SectorSize - (2 + SecondIndex) * sizeof(int) - 3 * DataForm * sizeof(char) - 1) / sizeof(int)) //filehdr's sector index maximum num
//#define MaxFileSize 	(NumDirect * SectorSize)

//file maximum size 6+5*32=166(sectors)
#define TotalDirect NumDirect + SecondIndex * SecondDirect
// ****lab5 code end****

// The following class defines the Nachos "file header" (in UNIX terms,
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks.
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.

class FileHeader {
  public:
    bool Allocate(BitMap *bitMap, int fileSize);// Initialize a file header,
						//  including allocating space
						//  on disk for the file data
    void Deallocate(BitMap *bitMap);  		// De-allocate this file's
						//  data blocks

    void FetchFrom(int sectorNumber); 	// Initialize file header from disk
    void WriteBack(int sectorNumber); 	// Write modifications to file header
					//  back to disk

    int ByteToSector(int offset);	// Convert a byte offset into the file
					// to the disk sector containing
					// the byte

    int FileLength();			// Return the length of the file
					// in bytes

    void Print();			// Print the contents of the file.
// ****lab5 code begin****
private:
    char createTime[DataForm];
    char updateTime[DataForm];
    char lastOpenTime[DataForm];
public:
    char* getCreateTime() {return createTime;}
    char* getUpdateTime() {return updateTime;}
    char* getLastOpenTime() {return lastOpenTime;}
    void setCreateTime();
    void setUpdateTime();
    void setLastOpenTime();
  private:
    int numBytes;			// Number of bytes in the file
    int numSectors;			// Number of data sectors in the file
    int dataSectors[TotalDirect];		// Disk sector numbers for each data
					// block in the file
// ****lab5 code end****
};

#endif // FILEHDR_H
