#pragma once
#include <stdexcept>
#include <string>

namespace lms {

// Base class for every error the application raises on purpose.
class LmsError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
class ValidationError : public LmsError { public: using LmsError::LmsError; };
class AuthenticationError : public LmsError { public: using LmsError::LmsError; };
class AuthorizationError : public LmsError { public: using LmsError::LmsError; };
class NotFoundError : public LmsError { public: using LmsError::LmsError; };
class InvalidTransitionError : public LmsError { public: using LmsError::LmsError; };
class DatabaseError : public LmsError { public: using LmsError::LmsError; };

}  // namespace lms
