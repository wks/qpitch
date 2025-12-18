#include "tst_cyclicbuffertest.h"

#include "cyclicbuffer.h"

#include <algorithm>
#include <QString>

QTEST_MAIN(TestCyclicBuffer)

static unsigned char iotaBuffer[256];

static void checkLastBytes(const CyclicBuffer &buffer, size_t len, size_t actualCopy,
                           size_t iotaStart)
{
    unsigned char resultBuffer[256]{ 0 };
    size_t actualCopied = buffer.copyLastBytes(resultBuffer, len);
    QCOMPARE(actualCopied, actualCopy);

    for (size_t i = 0; i < actualCopy; i++) {
        size_t iotaIndex = iotaStart + i;
        QVERIFY2(resultBuffer[i] == iotaBuffer[iotaIndex],
                 qPrintable(QString("different at buffer %1 iota %2: %3 != %4 len: %5, actualCopy: "
                                    "%6, iotaStart: %7")
                                    .arg(i)
                                    .arg(iotaIndex)
                                    .arg(resultBuffer[i])
                                    .arg(iotaBuffer[iotaIndex])
                                    .arg(len)
                                    .arg(actualCopy)
                                    .arg(iotaStart)));
    }
}

void TestCyclicBuffer::initTestCase()
{
    std::iota(iotaBuffer, iotaBuffer + 256, 0);
}

void TestCyclicBuffer::testShortAppend()
{
    CyclicBuffer buffer(47);
    buffer.append(iotaBuffer, 30);

    checkLastBytes(buffer, 10, 10, 30 - 10);
    checkLastBytes(buffer, 30, 30, 0);
    checkLastBytes(buffer, 50, 30, 0);
}

void TestCyclicBuffer::testShortShortToMedium()
{
    CyclicBuffer buffer(47);
    buffer.append(iotaBuffer, 30);
    buffer.append(iotaBuffer + 30, 17);

    checkLastBytes(buffer, 10, 10, 47 - 10);
    checkLastBytes(buffer, 30, 30, 47 - 30);
    checkLastBytes(buffer, 47, 47, 47 - 47);
    checkLastBytes(buffer, 50, 47, 47 - 47);
    checkLastBytes(buffer, 70, 47, 47 - 47);
}

void TestCyclicBuffer::testShortShortToMediumThenShort()
{
    CyclicBuffer buffer(47);
    buffer.append(iotaBuffer, 30);
    buffer.append(iotaBuffer + 30, 17);
    buffer.append(iotaBuffer + 47, 13);

    checkLastBytes(buffer, 10, 10, 60 - 10);
    checkLastBytes(buffer, 30, 30, 60 - 30);
    checkLastBytes(buffer, 47, 47, 60 - 47);
    checkLastBytes(buffer, 50, 47, 60 - 47);
    checkLastBytes(buffer, 60, 47, 60 - 47);
    checkLastBytes(buffer, 70, 47, 60 - 47);
}
void TestCyclicBuffer::testShortShortToLong()
{
    CyclicBuffer buffer(47);
    buffer.append(iotaBuffer, 30);
    buffer.append(iotaBuffer + 30, 20);

    checkLastBytes(buffer, 10, 10, 50 - 10);
    checkLastBytes(buffer, 30, 30, 50 - 30);
    checkLastBytes(buffer, 47, 47, 50 - 47);
    checkLastBytes(buffer, 50, 47, 50 - 47);
    checkLastBytes(buffer, 70, 47, 50 - 47);
}

void TestCyclicBuffer::testMediumAppend()
{
    CyclicBuffer buffer(47);
    buffer.append(iotaBuffer, 47);

    checkLastBytes(buffer, 10, 10, 47 - 10);
    checkLastBytes(buffer, 30, 30, 47 - 30);
    checkLastBytes(buffer, 47, 47, 47 - 47);
    checkLastBytes(buffer, 50, 47, 47 - 47);
}

void TestCyclicBuffer::testLongAppend()
{
    CyclicBuffer buffer(47);
    buffer.append(iotaBuffer, 60);

    checkLastBytes(buffer, 10, 10, 60 - 10);
    checkLastBytes(buffer, 30, 30, 60 - 30);
    checkLastBytes(buffer, 47, 47, 60 - 47);
    checkLastBytes(buffer, 50, 47, 60 - 47);
    checkLastBytes(buffer, 60, 47, 60 - 47);
    checkLastBytes(buffer, 70, 47, 60 - 47);
}

void TestCyclicBuffer::testShortLongAppend()
{
    CyclicBuffer buffer(47);
    buffer.append(iotaBuffer, 30);
    buffer.append(iotaBuffer + 30, 60);

    checkLastBytes(buffer, 10, 10, 90 - 10);
    checkLastBytes(buffer, 30, 30, 90 - 30);
    checkLastBytes(buffer, 47, 47, 90 - 47);
    checkLastBytes(buffer, 60, 47, 90 - 47);
    checkLastBytes(buffer, 90, 47, 90 - 47);
    checkLastBytes(buffer, 99, 47, 90 - 47);
}
