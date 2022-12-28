file(READ ${INPUT} data)
string(REGEX REPLACE
	"new ([a-zA-Z_:]+)"
	"di.template create<\\1>"
	data
	"${data}")
string(REGEX REPLACE
	"void setupUi[(]([a-zA-Z_]+) [*]([a-zA-Z_]+)[)][\r\n\t ]*[{]"
	"template <class Tuple = typename std::tuple<>> void setupUi(\\1 *\\2, const Tuple& tuple = {})
    {
        const FreeHeroes::Resolver<Tuple> di(tuple);\n"
	data
	"${data}")
string(REPLACE
	"QT_BEGIN_NAMESPACE"
	"#include \"DependencyInjector.hpp\"\nQT_BEGIN_NAMESPACE"
	data
	"${data}"
	)
file(WRITE ${INPUT} "${data}")
