#include <QTest>
#include <QObject>

class TestCyclicBuffer : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void testShortAppend();
    void testShortShortToMedium();
    void testShortShortToMediumThenShort();
    void testShortShortToLong();
    void testMediumAppend();
    void testLongAppend();
    void testShortLongAppend();
};
