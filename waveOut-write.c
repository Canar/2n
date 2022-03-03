#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

typedef struct wavFileHeader
{
    long chunkId;           //"RIFF" (0x52,0x49,0x46,0x46)
    long chunkSize;         // (fileSize - 8)  - could also be thought of as bytes of data in file following this field (bytesRemaining)
    long riffType;          // "WAVE" (0x57415645)
} wavFileHeader;

typedef struct fmtChunk
{
    long chunkId;                       // "fmt " - (0x666D7420)
    long chunkDataSize;                 // 16 + extra format bytes
    short compressionCode;              // 1 - 65535
    short numChannels;                  // 1 - 65535
    long sampleRate;                    // 1 - 0xFFFFFFFF
    long avgBytesPerSec;                // 1 - 0xFFFFFFFF
    short blockAlign;                   // 1 - 65535
    short significantBitsPerSample;     // 2 - 65535
    short extraFormatBytes;             // 0 - 65535
} fmtChunk;

typedef struct wavChunk
{
    long chunkId;
    long chunkDataSize;
} wavChunk;

int main(int argc,char** argv)
{
    char *buffer;
    long fileSize;

    FILE *fp = fopen(argv[1], "rb");
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buffer = (char*) calloc(1, fileSize+1);
    fread(buffer, 1, fileSize, fp);
    fclose(fp);

    //parseWav(buffer);

    long *mPtr;
    void *tmpPtr;

    char *parsebuf;

    WAVEFORMATEX wf;
    WAVEHDR wh;
    HWAVEOUT hWaveOut;

    struct fmtChunk mFmtChunk;
    struct wavChunk mDataChunk;

    mPtr = (long*)buffer;

    if ( mPtr[0] == 0x46464952) //  little endian check for 'RIFF'
    {
        mPtr += 3;
        if (mPtr[0] == 0x20746D66)  // little endian for "fmt "
        {
           // printf("Format chunk found\n");

            tmpPtr = mPtr;
            memcpy(&mFmtChunk, tmpPtr, sizeof(mFmtChunk));
            tmpPtr += 8;
            tmpPtr += mFmtChunk.chunkDataSize;

            mPtr = (long*)tmpPtr;
            if (mPtr[0] == 0x61746164)        // little endian for "data"
            {
            //    printf("Data chunk found\n");

                tmpPtr = mPtr;
                memcpy(&mDataChunk, tmpPtr, sizeof(mDataChunk));
                mPtr += 2;

                parsebuf = (char*) malloc(mDataChunk.chunkDataSize);
                memcpy(parsebuf, mPtr, mDataChunk.chunkDataSize);

                printf("sampleRate: %d\n", mFmtChunk.sampleRate);

                wf.wFormatTag = mFmtChunk.compressionCode;
                wf.nChannels = mFmtChunk.numChannels;
                wf.nSamplesPerSec = mFmtChunk.sampleRate;
                wf.nAvgBytesPerSec = mFmtChunk.avgBytesPerSec;
                wf.nBlockAlign = mFmtChunk.blockAlign;
                wf.wBitsPerSample = mFmtChunk.significantBitsPerSample;
                wf.cbSize = mFmtChunk.extraFormatBytes;

                wh.lpData = buffer;
                wh.dwBufferLength = mDataChunk.chunkDataSize;
                wh.dwFlags = 0;
                wh.dwLoops = 0;

                waveOutOpen(&hWaveOut,WAVE_MAPPER,&wf,0,0,CALLBACK_NULL);
                waveOutPrepareHeader(hWaveOut,&wh,sizeof(wh));
                waveOutWrite(hWaveOut,&wh,sizeof(wh));

                do {}
                while (!(wh.dwFlags & WHDR_DONE));

                waveOutUnprepareHeader(hWaveOut,&wh,sizeof(wh));
                waveOutClose(hWaveOut);

                free(parsebuf);
            }
        }

    }

    else
        printf("INvalid WAV\n");

    free(buffer);

    return 0;
}
