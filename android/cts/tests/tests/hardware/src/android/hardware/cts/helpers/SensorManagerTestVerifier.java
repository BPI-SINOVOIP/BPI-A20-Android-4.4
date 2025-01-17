/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.cts.helpers;

import android.content.Context;

import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener2;
import android.hardware.SensorManager;
import android.hardware.TriggerEvent;
import android.hardware.TriggerEventListener;
import android.test.AndroidTestCase;

import java.io.Closeable;

import java.util.concurrent.ConcurrentLinkedDeque;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import junit.framework.Assert;

/**
 * Test class to wrap SensorManager with verifications and test checks.
 * This class allows to perform operations in the Sensor Manager and performs all the expected test
 * verification on behalf of th owner.
 * An object can be used to quickly writing tests that focus on the scenario that needs to be
 * verified, and not in the implicit verifications that need to take place at any step.
 */
public class SensorManagerTestVerifier implements Closeable {
    private final int WAIT_TIMEOUT_IN_SECONDS = 30;

    private final Assert mAssert;
    private final SensorManager mSensorManager;
    private final Sensor mSensorUnderTest;
    private final int mSamplingRateInUs;
    private final int mReportLatencyInUs;

    private TestSensorListener mEventListener;

    /**
     * Construction methods.
     */
    public SensorManagerTestVerifier(
            AndroidTestCase testCase,
            int sensorType,
            int samplingRateInUs,
            int reportLatencyInUs) {
        mAssert = testCase;

        mSensorManager = (SensorManager)testCase.getContext().getSystemService(
                Context.SENSOR_SERVICE);
        mSensorUnderTest = SensorCtsHelper.getSensor(testCase, sensorType);
        mSamplingRateInUs = samplingRateInUs;
        mReportLatencyInUs = reportLatencyInUs;

        mEventListener = new TestSensorListener(mSensorUnderTest);
    }

    /**
     * Members
     */
    public void close() {
        this.unregisterListener();
        mEventListener = null;
    }

    public Sensor getUnderlyingSensor() {
        return mSensorUnderTest;
    }

    public void registerListener(String debugInfo) {
        boolean result = mSensorManager.registerListener(
                mEventListener,
                mSensorUnderTest,
                mSamplingRateInUs,
                mReportLatencyInUs);
        String message = SensorCtsHelper.formatAssertionMessage(
                "registerListener",
                mSensorUnderTest,
                debugInfo);
        mAssert.assertTrue(message, result);
    }

    public void registerListener() {
        this.registerListener("");
    }

    public void unregisterListener() {
        mSensorManager.unregisterListener(mEventListener, mSensorUnderTest);
    }

    public TestSensorEvent[] getEvents(int count, String debugInfo) {
        mEventListener.waitForEvents(count, debugInfo);
        TestSensorEvent[] events = mEventListener.getAllEvents();
        mEventListener.clearEvents();

        return events;
    }

    public TestSensorEvent[] getEvents(int count) {
        return this.getEvents(count, "");
    }

    public TestSensorEvent[] collectEvents(int eventCount, String debugInfo) {
        this.registerListener(debugInfo);
        TestSensorEvent[] events = this.getEvents(eventCount, debugInfo);
        this.unregisterListener();

        return events;
    }

    public TestSensorEvent[] collectEvents(int eventCount) {
        return this.collectEvents(eventCount, "");
    }

    public void startFlush() {
        String message = SensorCtsHelper.formatAssertionMessage(
                "Flush",
                mSensorUnderTest,
                "" /* format */);
        mAssert.assertTrue(message, mSensorManager.flush(mEventListener));
    }

    public void waitForFlush() throws InterruptedException {
        mEventListener.waitForFlushComplete();
    }

    public void flush() throws InterruptedException {
        this.startFlush();
        this.waitForFlush();
    }

    /**
     * Definition of support test classes.
     */
    private class TestSensorListener implements SensorEventListener2 {
        private final Sensor mSensorUnderTest;

        private final ConcurrentLinkedDeque<TestSensorEvent> mSensorEventsList =
                new ConcurrentLinkedDeque<TestSensorEvent>();

        private volatile CountDownLatch mEventLatch;
        private volatile CountDownLatch mFlushLatch = new CountDownLatch(1);

        public TestSensorListener(Sensor sensor) {
            mSensorUnderTest = sensor;
        }

        @Override
        public void onSensorChanged(SensorEvent event) {
            CountDownLatch latch = mEventLatch;
            if(latch != null) {
                // copy the event because there is no better way to do this in the platform
                mSensorEventsList.addLast(new TestSensorEvent(event));
                latch.countDown();
            }
        }

        @Override
        public void onAccuracyChanged(Sensor sensor, int accuracy) {
        }

        @Override
        public void onFlushCompleted(Sensor sensor) {
            CountDownLatch latch = mFlushLatch;
            mFlushLatch = new CountDownLatch(1);

            if(latch != null) {
                latch.countDown();
            }
        }

        public void waitForFlushComplete() throws InterruptedException {
            CountDownLatch latch = mFlushLatch;
            if(latch != null) {
                String message = SensorCtsHelper.formatAssertionMessage(
                        "WaitForFlush",
                        mSensorUnderTest,
                        "" /* format */);
                mAssert.assertTrue(message, latch.await(WAIT_TIMEOUT_IN_SECONDS, TimeUnit.SECONDS));
            }
        }

        public void waitForEvents(int eventCount) {
            waitForEvents(eventCount, "");
        }

        public void waitForEvents(int eventCount, String timeoutInfo) {
            mEventLatch = new CountDownLatch(eventCount);
            try {
                boolean awaitCompleted = mEventLatch.await(WAIT_TIMEOUT_IN_SECONDS, TimeUnit.SECONDS);
                // TODO: can we collect bug reports on error based only if needed? env var?

                String message = SensorCtsHelper.formatAssertionMessage(
                        "WaitForEvents",
                        mSensorUnderTest,
                        "count:%d, available:%d, %s",
                        eventCount,
                        mSensorEventsList.size(),
                        timeoutInfo);
                mAssert.assertTrue(message, awaitCompleted);
            } catch(InterruptedException e) {
            } finally {
                mEventLatch = null;
            }
        }

        public TestSensorEvent getLastEvent() {
            return mSensorEventsList.getLast();
        }

        public TestSensorEvent[] getAllEvents() {
            return mSensorEventsList.toArray(new TestSensorEvent[0]);
        }

        public void clearEvents() {
            mSensorEventsList.clear();
        }
    }

    public class TestTriggerListener extends TriggerEventListener {
        @Override
        public void onTrigger(TriggerEvent event) {
        }
    }
}
