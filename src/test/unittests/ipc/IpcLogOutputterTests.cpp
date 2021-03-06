/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define TEST_ENV

#include "test/mock/ipc/MockIpcServer.h"

#include "mt/Thread.h"
#include "ipc/IpcLogOutputter.h"
#include "base/String.h"
#include "common/common.h"

#include "test/global/gmock.h"
#include "test/global/gtest.h"

// HACK: ipc logging only used on windows anyway
#if WINAPI_MSWINDOWS

using ::testing::_;
using ::testing::Return;
using ::testing::Matcher;
using ::testing::MatcherCast;
using ::testing::Property;
using ::testing::StrEq;

using namespace synergy;

inline const Matcher<const IpcMessage&> IpcLogLineMessageEq(const String& s) {
	const Matcher<const IpcLogLineMessage&> m(
		Property(&IpcLogLineMessage::logLine, StrEq(s)));
	return MatcherCast<const IpcMessage&>(m);
}

TEST(IpcLogOutputterTests, write_threadingEnabled_bufferIsSent)
{
	MockIpcServer mockServer;
	mockServer.delegateToFake();
	
	ON_CALL(mockServer, hasClients(_)).WillByDefault(Return(true));

	EXPECT_CALL(mockServer, hasClients(_)).Times(4);
	EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock 1\n"), _)).Times(1);
	EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock 2\n"), _)).Times(1);

	IpcLogOutputter outputter(mockServer, true);
	outputter.write(kNOTE, "mock 1");
	mockServer.waitForSend();
	outputter.write(kNOTE, "mock 2");
	mockServer.waitForSend();
}

TEST(IpcLogOutputterTests, write_overBufferMaxSize_firstLineTruncated)
{
	MockIpcServer mockServer;
	
	ON_CALL(mockServer, hasClients(_)).WillByDefault(Return(true));

	EXPECT_CALL(mockServer, hasClients(_)).Times(4);
	EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock 2\nmock 3\n"), _)).Times(1);

	IpcLogOutputter outputter(mockServer, false);
	outputter.bufferMaxSize(2);

	// log more lines than the buffer can contain
	outputter.write(kNOTE, "mock 1");
	outputter.write(kNOTE, "mock 2");
	outputter.write(kNOTE, "mock 3");
	outputter.sendBuffer();
}

TEST(IpcLogOutputterTests, write_underBufferMaxSize_allLinesAreSent)
{
	MockIpcServer mockServer;
	
	ON_CALL(mockServer, hasClients(_)).WillByDefault(Return(true));

	EXPECT_CALL(mockServer, hasClients(_)).Times(3);
	EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock 1\nmock 2\n"), _)).Times(1);

	IpcLogOutputter outputter(mockServer, false);
	outputter.bufferMaxSize(2);

	// log more lines than the buffer can contain
	outputter.write(kNOTE, "mock 1");
	outputter.write(kNOTE, "mock 2");
	outputter.sendBuffer();
}

TEST(IpcLogOutputterTests, write_overBufferRateLimit_lastLineTruncated)
{
	MockIpcServer mockServer;
	
	ON_CALL(mockServer, hasClients(_)).WillByDefault(Return(true));

	EXPECT_CALL(mockServer, hasClients(_)).Times(6);
	EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock 1\n"), _)).Times(1);
	EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock 3\n"), _)).Times(1);

	IpcLogOutputter outputter(mockServer, false);
	outputter.bufferRateLimit(1, 0.001); // 1ms

	// log 1 more line than the buffer can accept in time limit.
	outputter.write(kNOTE, "mock 1");
	outputter.write(kNOTE, "mock 2");
	outputter.sendBuffer();
	
	// after waiting the time limit send another to make sure
	// we can log after the time limit passes.
	// HACK: sleep causes the unit test to fail intermittently,
	// so lets try 100ms (there must be a better way to solve this)
	ARCH->sleep(0.1); // 100ms
	outputter.write(kNOTE, "mock 3");
	outputter.write(kNOTE, "mock 4");
	outputter.sendBuffer();
}

TEST(IpcLogOutputterTests, write_underBufferRateLimit_allLinesAreSent)
{
	MockIpcServer mockServer;
	
	ON_CALL(mockServer, hasClients(_)).WillByDefault(Return(true));

	EXPECT_CALL(mockServer, hasClients(_)).Times(6);
	EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock 1\nmock 2\n"), _)).Times(1);
	EXPECT_CALL(mockServer, send(IpcLogLineMessageEq("mock 3\nmock 4\n"), _)).Times(1);

	IpcLogOutputter outputter(mockServer, false);
	outputter.bufferRateLimit(4, 1); // 1s (should be plenty of time)

	// log 1 more line than the buffer can accept in time limit.
	outputter.write(kNOTE, "mock 1");
	outputter.write(kNOTE, "mock 2");
	outputter.sendBuffer();
	
	// after waiting the time limit send another to make sure
	// we can log after the time limit passes.
	outputter.write(kNOTE, "mock 3");
	outputter.write(kNOTE, "mock 4");
	outputter.sendBuffer();
}

#endif // WINAPI_MSWINDOWS
