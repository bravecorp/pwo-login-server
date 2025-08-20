/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef DATABASE_DBRESULT
#define DATABASE_DBRESULT

#include <boost/lexical_cast.hpp>

class DBResult {
    public:
        explicit DBResult(MYSQL_RES* res);
        ~DBResult() = default;

        // non-copyable
        DBResult(const DBResult&) = delete;
        DBResult& operator=(const DBResult&) = delete;

        void next() {
            m_index++;
        }
        bool hasNext() {
            return (m_index < m_rows.size());
        }

        std::string getString(const std::string& field);

        template<typename T>
        T getNumber(const std::string& field)
        {
            if (m_rows.empty() || m_index >= m_rows.size()) {
                return T{};
            }

            auto it = m_fields.find(field);
            if (it == m_fields.end()) {
                return T{};
            }

            auto row = m_rows[m_index];
            try {
                return boost::lexical_cast<T>(row[it->second]);
            } catch (const boost::bad_lexical_cast&) {
                return T{};
            }
        }


    private:
        std::map<std::string, size_t> m_fields;
        std::vector<MYSQL_ROW> m_rows;
        size_t m_index = 0;
};

#endif
