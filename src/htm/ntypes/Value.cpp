/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2013, Numenta, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 * --------------------------------------------------------------------- */

/** @file
 * Implementation of the Value class
 */

#include <htm/ntypes/Value.hpp>
#include <htm/utils/Log.hpp>

using namespace htm;

Value::Value() { 
  category_ = noCategory; 
}

Value::Value(std::shared_ptr<Scalar> &s) {
  category_ = scalarCategory;
  scalar_ = s;
}

Value::Value(std::shared_ptr<Array> &a) {
  category_ = arrayCategory;
  array_ = a;
}

Value::Value(const std::string& s) {
  category_ = stringCategory;
  string_ = s;
}

bool Value::isScalar() const { return category_ == scalarCategory; }

bool Value::isArray() const { return category_ == arrayCategory; }

bool Value::isString() const { return category_ == stringCategory; }

NTA_BasicType Value::getType() const {
  switch (category_) {
  case scalarCategory:
    return scalar_->getType();
    break;
  case arrayCategory:
    return array_->getType();
    break;
  case stringCategory:
    // string
    return NTA_BasicType_Byte;
    break;
  default:
    NTA_THROW << "Value::getType() Value has no type.";
  }
}

std::shared_ptr<Scalar> Value::getScalar() const {
  NTA_CHECK(category_ == scalarCategory);
  return scalar_;
}

std::shared_ptr<Array> Value::getArray() const {
  NTA_CHECK(category_ == arrayCategory);
  return array_;
}

std::string Value::getString() const {
  NTA_CHECK(category_ == stringCategory);
  return string_;
}

template <typename T> T Value::getScalarT() const {
  NTA_CHECK(category_ == scalarCategory);
  if (BasicType::getType<T>() != scalar_->getType()) {
    NTA_THROW << "Attempt to access scalar of type "
              << BasicType::getName(scalar_->getType()) << " as type "
              << BasicType::getName<T>();
  }
  return scalar_->getValue<T>();
}

const std::string Value::getDescription() const {
  switch (category_) {
  case stringCategory:
    return std::string("string") + " (" + string_ + ")";
    break;
  case scalarCategory:
    return std::string("Scalar of type ") + BasicType::getName(scalar_->getType());
    break;
  case arrayCategory:
    return std::string("Array of type ") +  BasicType::getName(array_->getType());
    break;
  case noCategory:
    return std::string("Type undefined");
    break;
  }
  return "NOT REACHED";
}

void ValueMap::add(const std::string &key, const Value &value) {
  if (map_.find(key) != map_.end()) {
    NTA_THROW << "Key '" << key << "' specified twice";
  }
  auto vp = std::make_shared<Value>(value);

  map_.insert(std::make_pair(key, vp));
}

Value::Category Value::getCategory() const { return category_; }

ValueMap::const_iterator ValueMap::begin() const { return map_.begin(); }

ValueMap::const_iterator ValueMap::end() const { return map_.end(); }

// specializations of getValue()
// gcc 4.2 complains if they are not inside the namespace declaration
namespace htm {
template Byte Value::getScalarT<Byte>() const;
template Int16 Value::getScalarT<Int16>() const;
template Int32 Value::getScalarT<Int32>() const;
template Int64 Value::getScalarT<Int64>() const;
template UInt16 Value::getScalarT<UInt16>() const;
template UInt32 Value::getScalarT<UInt32>() const;
template UInt64 Value::getScalarT<UInt64>() const;
template Real32 Value::getScalarT<Real32>() const;
template Real64 Value::getScalarT<Real64>() const;
template Handle Value::getScalarT<Handle>() const;
template bool Value::getScalarT<bool>() const;
} // namespace htm

ValueMap::ValueMap(){};

ValueMap::~ValueMap() {
  map_.clear();
}

ValueMap::ValueMap(const ValueMap &rhs) {
  map_.clear();

  for (const auto &rh : rhs) {
    map_.insert(std::make_pair(rh.first, rh.second));
  }
}

void ValueMap::dump() const {
  NTA_DEBUG << "===== Value Map:";
  for (const auto &elem : map_) {
    std::string key = elem.first;
    std::shared_ptr<Value> value = elem.second;
    NTA_DEBUG << "key: " << key
              << " datatype: " << BasicType::getName(value->getType())
              << " category: " << value->getCategory();
  }
  NTA_DEBUG << "===== End of Value Map";
}

bool ValueMap::contains(const std::string &key) const {
  return (map_.find(key) != map_.end());
}

std::shared_ptr<Value> ValueMap::getValue(const std::string &key) const {
  auto item = map_.find(key);
  if (item == map_.end()) {
    NTA_THROW << "No value '" << key << "' found in Value Map";
  }
  return (item->second);
}

template <typename T>
T ValueMap::getScalarT(const std::string &key, T defaultValue) const {
  auto item = map_.find(key);
  if (item == map_.end()) {
    return defaultValue;
  } else {
    return getScalarT<T>(key);
  }
}

template <typename T> T ValueMap::getScalarT(const std::string &key) const {
  std::shared_ptr<Scalar> s = getScalar(key);
  if (s->getType() != BasicType::getType<T>()) {
    NTA_THROW << "Invalid attempt to access parameter '" << key
      << "' as type a " << BasicType::getName<T>()
      << " but the Spec defines it as type " << BasicType::getName(s->getType());
  }

  return s->getValue<T>();
}


void ValueMap::assign(const std::string &key, char *ptr, NTA_BasicType type) const {
  std::shared_ptr<Scalar> s = getScalar(key);
  if (s->getType() != type) {
    NTA_THROW << "Invalid attempt to access parameter '" << key
              << "' as type a " << BasicType::getName(type)
              << " but the Spec defines it as type "
              << BasicType::getName(s->getType());
  }
  s->assign(ptr, type);
}


std::shared_ptr<Array> ValueMap::getArray(const std::string &key) const {
  std::shared_ptr<Value> v = getValue(key);
  if (!v->isArray()) {
    NTA_THROW << "Attempt to access element '" << key
              << "' of value map as an array but it is a '"
              << v->getDescription();
  }
  return v->getArray();
}

std::shared_ptr<Scalar> ValueMap::getScalar(const std::string &key) const {
  std::shared_ptr<Value> v = getValue(key);
  if (!v->isScalar()) {
    NTA_THROW << "Attempt to access element '" << key
              << "' of value map as an array but it is a '"
              << v->getDescription();
  }
  return v->getScalar();
}

std::string ValueMap::getString(const std::string& key) const {
  std::shared_ptr<Value> v = getValue(key);
  if (! v->isString())
  {
    NTA_THROW << "Attempt to access element '" << key
              << "' of value map as a string but it is a '"
              << v->getDescription();
  }
  return v->getString();
}
std::string ValueMap::getString(const std::string &key, const std::string defaultValue) const {
  auto item = map_.find(key);
  if (item == map_.end()) {
    return defaultValue;
  } else {
    std::shared_ptr<Value> v = getValue(key);
    if (!v->isString()) {
      NTA_THROW << "Attempt to access element '" << key
                << "' of value map as a string but it is a '"
                << v->getDescription();
    }
    return v->getString();
  }
}


// explicit instantiations of getScalarT
namespace htm {
template Byte ValueMap::getScalarT(const std::string &key,
                                   Byte defaultValue) const;
template UInt16 ValueMap::getScalarT(const std::string &key,
                                     UInt16 defaultValue) const;
template Int16 ValueMap::getScalarT(const std::string &key,
                                    Int16 defaultValue) const;
template UInt32 ValueMap::getScalarT(const std::string &key,
                                     UInt32 defaultValue) const;
template Int32 ValueMap::getScalarT(const std::string &key,
                                    Int32 defaultValue) const;
template UInt64 ValueMap::getScalarT(const std::string &key,
                                     UInt64 defaultValue) const;
template Int64 ValueMap::getScalarT(const std::string &key,
                                    Int64 defaultValue) const;
template Real32 ValueMap::getScalarT(const std::string &key,
                                     Real32 defaultValue) const;
template Real64 ValueMap::getScalarT(const std::string &key,
                                     Real64 defaultValue) const;
template Handle ValueMap::getScalarT(const std::string &key,
                                     Handle defaultValue) const;
template bool ValueMap::getScalarT(const std::string &key,
                                   bool defaultValue) const;

template Byte ValueMap::getScalarT(const std::string &key) const;
template UInt16 ValueMap::getScalarT(const std::string &key) const;
template Int16 ValueMap::getScalarT(const std::string &key) const;
template UInt32 ValueMap::getScalarT(const std::string &key) const;
template Int32 ValueMap::getScalarT(const std::string &key) const;
template UInt64 ValueMap::getScalarT(const std::string &key) const;
template Int64 ValueMap::getScalarT(const std::string &key) const;
template Real32 ValueMap::getScalarT(const std::string &key) const;
template Real64 ValueMap::getScalarT(const std::string &key) const;
template Handle ValueMap::getScalarT(const std::string &key) const;
template bool ValueMap::getScalarT(const std::string &key) const;

} // namespace htm
