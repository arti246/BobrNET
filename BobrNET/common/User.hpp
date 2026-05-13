class User {
public:
	User() = default;
	User(int id, const std::string& login, const std::string& password_hash)
		: m_id(id), m_login(login), m_password_hash(password_hash) {
	}6

	int id() const { return m_id; }
	const std::string& login() const { return m_login; }
	const std::string& password_hash() const { return m_password_hash; }

private:
	int m_id = -1;
	std::string m_login;
	std::string m_password_hash;
};