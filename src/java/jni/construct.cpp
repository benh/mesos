#include <jni.h>

#include <glog/logging.h>

#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <string>
#include <map>

#include <mesos/mesos.hpp>

#include "construct.hpp"

using namespace mesos;

using std::map;
using std::string;


template <typename T>
T parse(const void* data, int size)
{
  // This should always get called with data that can be parsed (i.e.,
  // ParseFromZeroCopyStream should never return false) because we
  // have static type checking in Java and C++. A dynamic language
  // will not have this luxury.
  google::protobuf::io::ArrayInputStream stream(data, size);
  T t;
  bool parsed = t.ParseFromZeroCopyStream(&stream);
  CHECK(parsed) << "Unexpected failure while parsing protobuf";
  return t;
}


template <>
string construct(JNIEnv* env, jobject jobj)
{
  string s;
  jstring jstr = (jstring) jobj;
  const char *str = (const char *) env->GetStringUTFChars(jstr, NULL);
  s = str;
  env->ReleaseStringUTFChars(jstr, str);
  return s;
}


template <>
map<string, string> construct(JNIEnv *env, jobject jobj)
{
  map<string, string> result;

  jclass clazz = env->GetObjectClass(jobj);

  // Set entrySet = map.entrySet();
  jmethodID entrySet = env->GetMethodID(clazz, "entrySet", "()Ljava/util/Set;");
  jobject jentrySet = env->CallObjectMethod(jobj, entrySet);

  clazz = env->GetObjectClass(jentrySet);

  // Iterator iterator = entrySet.iterator();
  jmethodID iterator = env->GetMethodID(clazz, "iterator", "()Ljava/util/Iterator;");
  jobject jiterator = env->CallObjectMethod(jentrySet, iterator);

  clazz = env->GetObjectClass(jiterator);

  // while (iterator.hasNext()) {
  jmethodID hasNext = env->GetMethodID(clazz, "hasNext", "()Z");

  jmethodID next = env->GetMethodID(clazz, "next", "()Ljava/lang/Object;");

  while (env->CallBooleanMethod(jiterator, hasNext)) {
    // Map.Entry entry = iterator.next();
    jobject jentry = env->CallObjectMethod(jiterator, next);

    clazz = env->GetObjectClass(jentry);

    // String key = entry.getKey();
    jmethodID getKey = env->GetMethodID(clazz, "getKey", "()Ljava/lang/Object;");
    jobject jkey = env->CallObjectMethod(jentry, getKey);

    // String value = entry.getValue();
    jmethodID getValue = env->GetMethodID(clazz, "getValue", "()Ljava/lang/Object;");
    jobject jvalue = env->CallObjectMethod(jentry, getValue);

    const string& key = construct<string>(env, jkey);
    const string& value = construct<string>(env, jvalue);

    result[key] = value;
  }

  return result;
}


template <>
Filters construct(JNIEnv* env, jobject jobj)
{
  jclass clazz = env->GetObjectClass(jobj);

  // byte[] data = obj.toByteArray();
  jmethodID toByteArray = env->GetMethodID(clazz, "toByteArray", "()[B");

  jbyteArray jdata = (jbyteArray) env->CallObjectMethod(jobj, toByteArray);

  jbyte* data = env->GetByteArrayElements(jdata, NULL);
  jsize length = env->GetArrayLength(jdata);

  const Filters& filters = parse<Filters>(data, length);

  env->ReleaseByteArrayElements(jdata, data, 0);

  return filters;
}


template <>
FrameworkID construct(JNIEnv* env, jobject jobj)
{
  jclass clazz = env->GetObjectClass(jobj);

  // byte[] data = obj.toByteArray();
  jmethodID toByteArray = env->GetMethodID(clazz, "toByteArray", "()[B");

  jbyteArray jdata = (jbyteArray) env->CallObjectMethod(jobj, toByteArray);

  jbyte* data = env->GetByteArrayElements(jdata, NULL);
  jsize length = env->GetArrayLength(jdata);

  const FrameworkID& frameworkId = parse<FrameworkID>(data, length);

  env->ReleaseByteArrayElements(jdata, data, 0);

  return frameworkId;
}


template <>
ExecutorID construct(JNIEnv* env, jobject jobj)
{
  jclass clazz = env->GetObjectClass(jobj);

  // byte[] data = obj.toByteArray();
  jmethodID toByteArray = env->GetMethodID(clazz, "toByteArray", "()[B");

  jbyteArray jdata = (jbyteArray) env->CallObjectMethod(jobj, toByteArray);

  jbyte* data = env->GetByteArrayElements(jdata, NULL);
  jsize length = env->GetArrayLength(jdata);

  const ExecutorID& executorId = parse<ExecutorID>(data, length);

  env->ReleaseByteArrayElements(jdata, data, 0);

  return executorId;
}


template <>
TaskID construct(JNIEnv* env, jobject jobj)
{
  jclass clazz = env->GetObjectClass(jobj);

  // byte[] data = obj.toByteArray();
  jmethodID toByteArray = env->GetMethodID(clazz, "toByteArray", "()[B");

  jbyteArray jdata = (jbyteArray) env->CallObjectMethod(jobj, toByteArray);

  jbyte* data = env->GetByteArrayElements(jdata, NULL);
  jsize length = env->GetArrayLength(jdata);

  const TaskID& taskId = parse<TaskID>(data, length);

  env->ReleaseByteArrayElements(jdata, data, 0);

  return taskId;
}


template <>
SlaveID construct(JNIEnv* env, jobject jobj)
{
  jclass clazz = env->GetObjectClass(jobj);

  // byte[] data = obj.toByteArray();
  jmethodID toByteArray = env->GetMethodID(clazz, "toByteArray", "()[B");

  jbyteArray jdata = (jbyteArray) env->CallObjectMethod(jobj, toByteArray);

  jbyte* data = env->GetByteArrayElements(jdata, NULL);
  jsize length = env->GetArrayLength(jdata);

  const SlaveID& slaveId = parse<SlaveID>(data, length);

  env->ReleaseByteArrayElements(jdata, data, 0);

  return slaveId;
}


template <>
OfferID construct(JNIEnv* env, jobject jobj)
{
  jclass clazz = env->GetObjectClass(jobj);

  // byte[] data = obj.toByteArray();
  jmethodID toByteArray = env->GetMethodID(clazz, "toByteArray", "()[B");

  jbyteArray jdata = (jbyteArray) env->CallObjectMethod(jobj, toByteArray);

  jbyte* data = env->GetByteArrayElements(jdata, NULL);
  jsize length = env->GetArrayLength(jdata);

  const OfferID& offerId = parse<OfferID>(data, length);

  env->ReleaseByteArrayElements(jdata, data, 0);

  return offerId;
}


template <>
TaskState construct(JNIEnv* env, jobject jobj)
{
  // int value = obj.getNumber(value);
  jclass clazz = env->FindClass("org/apache/mesos/Protos$TaskState");

  jmethodID getNumber = env->GetStaticMethodID(clazz, "getNumber", "()I");

  jint jvalue = env->CallIntMethod(jobj, getNumber);

  return (TaskState) jvalue;
}


template <>
TaskDescription construct(JNIEnv* env, jobject jobj)
{
  jclass clazz = env->GetObjectClass(jobj);

  // byte[] data = obj.toByteArray();
  jmethodID toByteArray = env->GetMethodID(clazz, "toByteArray", "()[B");

  jbyteArray jdata = (jbyteArray) env->CallObjectMethod(jobj, toByteArray);

  jbyte* data = env->GetByteArrayElements(jdata, NULL);
  jsize length = env->GetArrayLength(jdata);

  const TaskDescription& task = parse<TaskDescription>(data, length);

  env->ReleaseByteArrayElements(jdata, data, 0);

  return task;
}


template <>
TaskStatus construct(JNIEnv* env, jobject jobj)
{
  jclass clazz = env->GetObjectClass(jobj);

  // byte[] data = obj.toByteArray();
  jmethodID toByteArray = env->GetMethodID(clazz, "toByteArray", "()[B");

  jbyteArray jdata = (jbyteArray) env->CallObjectMethod(jobj, toByteArray);

  jbyte* data = env->GetByteArrayElements(jdata, NULL);
  jsize length = env->GetArrayLength(jdata);

  const TaskStatus& status = parse<TaskStatus>(data, length);

  env->ReleaseByteArrayElements(jdata, data, 0);

  return status;
}


template <>
ExecutorInfo construct(JNIEnv* env, jobject jobj)
{
  jclass clazz = env->GetObjectClass(jobj);

  // byte[] data = obj.toByteArray();
  jmethodID toByteArray = env->GetMethodID(clazz, "toByteArray", "()[B");

  jbyteArray jdata = (jbyteArray) env->CallObjectMethod(jobj, toByteArray);

  jbyte* data = env->GetByteArrayElements(jdata, NULL);
  jsize length = env->GetArrayLength(jdata);

  const ExecutorInfo& executor = parse<ExecutorInfo>(data, length);

  env->ReleaseByteArrayElements(jdata, data, 0);

  return executor;
}
