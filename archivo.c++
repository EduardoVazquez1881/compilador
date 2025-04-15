#include <iostream>
#include <regex>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <memory>
#include <stack>

using namespace std;

// Enumeración para los tipos de datos soportados por el lenguaje
enum class DataType {
    INT,      // Tipo entero
    FLOAT,    // Tipo punto flotante
    STRING,   // Tipo cadena de texto
    UNKNOWN   // Tipo desconocido o no determinado
};

// Pila semántica para gestionar tipos de datos
stack<DataType> pilaSemantica;

// Estructura para almacenar información de los tokens detectados en el análisis léxico
struct Token {
    string tipo;    // Categoría del token (VARIABLE, CICLO, OPERADOR, etc.)
    string valor;   // Contenido textual del token
};

// Estructura para almacenar información de variables en la tabla de símbolos
struct Simbolo {
    string nombre;       // Nombre de la variable
    string tipo;         // Tipo de dato ("int", "float", "string")
    string valor;        // Valor actual de la variable
    string id_contador;  // Identificador único para la variable
};

// Declaración adelantada de la función limpiarPilaSemantica
void limpiarPilaSemantica();

// Variables globales
vector<Simbolo> tablaSimbolos;  // Tabla de símbolos para almacenar información de variables
int idGlobalCounter = 1;        // Contador para generar IDs únicos para variables

// Clase base abstracta para nodos del árbol sintáctico abstracto (AST)
struct Node {
    // Método virtual para evaluar el valor numérico de un nodo
    virtual double evaluate(const vector<Simbolo>& tablaSimbolos) const = 0;
    
    // Método virtual para determinar el tipo de dato de un nodo
    virtual DataType getType(const vector<Simbolo>& tablaSimbolos) const = 0;
    
    // Destructor virtual para permitir liberación correcta de memoria
    virtual ~Node() {}
};

// Nodo para representar valores numéricos literales en el AST
struct NumberNode : public Node {
    double value;     // Valor numérico
    DataType dataType; // Tipo de dato (INT o FLOAT)
    
    // Constructor con valor y tipo explícito
    NumberNode(double val, DataType type) : value(val), dataType(type) {}
    
    // Constructor que infiere el tipo a partir del valor como cadena
    NumberNode(const string& val) {
        if (val.find('.') != string::npos) {
            // Si contiene punto decimal, es tipo FLOAT
            dataType = DataType::FLOAT;
            value = stod(val);
        } else {
            // Si no contiene punto decimal, es tipo INT
            dataType = DataType::INT;
            value = stod(val);
        }
    }
    
    // Implementación de evaluate: devuelve el valor numérico
    double evaluate(const vector<Simbolo>& tablaSimbolos) const override { 
        return value; 
    }
    
    // Implementación de getType: devuelve el tipo de dato
    DataType getType(const vector<Simbolo>& tablaSimbolos) const override {
        return dataType;
    }
};

// Nodo para representar variables en el AST
struct VariableNode : public Node {
    string name;  // Nombre de la variable
    
    // Constructor
    VariableNode(string varName) : name(varName) {}
    
    // Implementación de evaluate: busca el valor de la variable en la tabla de símbolos
    double evaluate(const vector<Simbolo>& tablaSimbolos) const override {
        // Busca el valor en la tabla de simbolos
        for (const auto& simbolo : tablaSimbolos) {
            if (simbolo.nombre == name) {
                return stod(simbolo.valor);
            }
        }
        throw runtime_error("Variable no encontrada: " + name);
    }
    
    // Implementación de getType: busca el tipo de la variable en la tabla de símbolos
    DataType getType(const vector<Simbolo>& tablaSimbolos) const override {
        // Busca el tipo en la tabla de simbolos
        for (const auto& simbolo : tablaSimbolos) {
            if (simbolo.nombre == name) {
                if (simbolo.tipo == "int") return DataType::INT;
                if (simbolo.tipo == "float") return DataType::FLOAT;
                if (simbolo.tipo == "string") return DataType::STRING;
            }
        }
        throw runtime_error("Variable no encontrada: " + name);
    }
};

// Nodo para representar operaciones aritméticas en el AST
struct OperatorNode : public Node {
    char op;                   // Operador aritmético (+, -, *, /)
    unique_ptr<Node> left, right;  // Nodos hijos izquierdo y derecho
    
    // Constructor
    OperatorNode(char oper, unique_ptr<Node> lhs, unique_ptr<Node> rhs)
        : op(oper), left(move(lhs)), right(move(rhs)) {}
    
    // Implementación de getType: verifica compatibilidad de tipos entre operandos
    DataType getType(const vector<Simbolo>& tablaSimbolos) const override {
        DataType leftType = left->getType(tablaSimbolos);
        DataType rightType = right->getType(tablaSimbolos);
    
        // Permitir operaciones entre int y float (convertir int a float)
        if ((leftType == DataType::INT && rightType == DataType::FLOAT) ||
            (leftType == DataType::FLOAT && rightType == DataType::INT)) {
            return DataType::FLOAT; // El resultado será float
        }
    
        // Comprobar que los tipos son compatibles
        if (leftType != rightType) {
            throw runtime_error("Error de tipo: no se pueden mezclar tipos diferentes en operaciones");
        }
    
        // Las operaciones con strings solo pueden ser concatenación (+)
        if (leftType == DataType::STRING && op != '+') {
            throw runtime_error("Error de tipo: solo se permite la concatenación (+) con strings");
        }
    
        return leftType;
    }
    
    // Implementación de evaluate: calcula el resultado de la operación aritmética
    double evaluate(const vector<Simbolo>& tablaSimbolos) const override {
        // Verificar los tipos antes de evaluar
        DataType operationType = getType(tablaSimbolos);
    
        // No permitimos operaciones con strings en evaluate (solo concatenación, que debería manejarse de otra manera)
        if (operationType == DataType::STRING) {
            throw runtime_error("Las operaciones con strings deben manejarse en otra función");
        }
    
        // Obtener los valores de los operandos
        double lval = left->evaluate(tablaSimbolos);
        double rval = right->evaluate(tablaSimbolos);
    
        // Realizar la operación según el operador
        switch (op) {
            case '+': return lval + rval;
            case '-': return lval - rval;
            case '*': return lval * rval;
            case '/': return lval / rval;
            default: throw runtime_error("Operador desconocido");
        }
    }
};

// Funciones auxiliares para el manejo de tipos

// Convierte una cadena que representa un tipo a su equivalente en DataType
DataType stringToDataType(const string& tipo) {
    if (tipo == "int") return DataType::INT;
    if (tipo == "float") return DataType::FLOAT;
    if (tipo == "string") return DataType::STRING;
    return DataType::UNKNOWN;
}

// Convierte un DataType a su representación como cadena
string dataTypeToString(DataType tipo) {
    switch (tipo) {
        case DataType::INT: return "int";
        case DataType::FLOAT: return "float";
        case DataType::STRING: return "string";
        default: return "unknown";
    }
}

// Verifica si un valor es compatible con un tipo de dato específico
bool esValorCompatible(const string& valor, const string& tipo) {
    if (tipo == "int") {
        // Verifica que sea un entero
        regex entero(R"(^-?\d+$)");
        return regex_match(valor, entero);
    } else if (tipo == "float") {
        // Verifica que sea un flotante o entero
        regex flotante(R"(^-?\d+(\.\d+)?$)");
        return regex_match(valor, flotante);
    } else if (tipo == "string") {
        // Cualquier valor es aceptable para string
        return true;
    }
    return false;
}

// Busca una variable en la tabla de símbolos por nombre
bool buscarVariable(const string& nombre, Simbolo& simbolo) {
    for (const auto& s : tablaSimbolos) {
        if (s.nombre == nombre) {
            simbolo = s;
            return true;
        }
    }
    return false;
}

// Busca el contenido de una variable
string buscarDato(const string& nombre){
    for (const auto& s : tablaSimbolos){
        if(s.nombre == nombre){
            return s.valor;
        }
    }
    throw runtime_error("Error: Variable '" + nombre + "' no declarada"); // Lanza una excepción si la variable no existe

}

// Realiza el análisis léxico del código fuente
vector<Token> Lexico(const string& input) {
    vector<Token> tokens;
    set<size_t> posiciones_procesadas;
    
    // Definición de patrones para reconocer diferentes elementos del lenguaje
    regex variables(R"(\b(int|float|string)\b)");
    regex ciclos(R"(\b(while)\b)");
    regex condicion(R"(\b(if|else)\b)");
    regex aritmeticos(R"([\+\-\/\*\*])");
    regex escritura(R"(\b(write)\b)");
    regex lectura(R"(\b(read)\b)");
    regex comparacion(R"((>=|<=|==|!=|>|<))");
    regex operador_regex(R"((=|;))");
    regex cadena(R"("[^"]*")"); // Reconocer cadenas entre comillas dobles
    regex par_der(R"(\))");   
    regex par_izq(R"(\()");   
    regex llave_der(R"(\})");   
    regex llave_izq(R"(\{)"); 
    regex cor_der(R"(\])");   
    regex cor_izq(R"(\[)");  
    regex numeros(R"(\d+(\.\d+)?)");
    regex identificadores(R"([a-zA-Z_][a-zA-Z0-9_]*)");
    
    // Vector de pares (patrón, tipo de token)
    vector<pair<regex, string>> patrones = {
        {variables, "VARIABLE"},
        {ciclos, "CICLO"},
        {escritura, "ESCRITURA"},
        {lectura, "LECTURA"},
        {comparacion, "COMPARACION"},
        {aritmeticos, "ARITMETICO"},
        {operador_regex, "OPERADOR"},
        {cadena, "CADENA"},
        {par_der, "PARENTESIS_DERECHO"},
        {par_izq, "PARENTESIS_IZQUIERDO"},
        {cor_der, "CORCHETE_DERECHO"},
        {cor_izq, "CORCHETE_IZQUIERDO"},
        {llave_der, "LLAVE_DERECHA"},
        {llave_izq, "LLAVE_IZQUIERDA"},
        {numeros, "NUMERO"},
        {condicion, "CONDICION"},
        {identificadores, "IDENTIFICADOR"}
    };
    
    // Procesar el texto de entrada para extraer tokens
    size_t pos = 0;
    while (pos < input.length()) {
        bool encontrado = false;
        for (auto& [patron, tipo] : patrones) {
            smatch match;
            if (regex_search(input.cbegin() + pos, input.cend(), match, patron) && match.position() == 0) {
                tokens.push_back({tipo, match.str()});
                pos += match.length();
                encontrado = true;
                break;
            }
        }
        if (!encontrado) {
            pos++;
        }
    }
    return tokens;
}

// Construye un árbol sintáctico abstracto (AST) a partir de tokens
unique_ptr<Node> construirAST(const vector<Token>& tokens, size_t& i) {
    // Primero parseamos el factor (número, variable o paréntesis)
    unique_ptr<Node> left;
    
    if (tokens[i].tipo == "PARENTESIS_IZQUIERDO") {
        i++; // Saltar '('
        left = construirAST(tokens, i); // Recursión para lo que está dentro
        if (i >= tokens.size() || tokens[i].tipo != "PARENTESIS_DERECHO") {
            throw runtime_error("Se esperaba ')'");
        }
        i++; // Saltar ')'
    } 
    else if (tokens[i].tipo == "NUMERO") {
        left = make_unique<NumberNode>(tokens[i].valor);
        pilaSemantica.push(left->getType(tablaSimbolos));
        i++;
    } 
    else if (tokens[i].tipo == "IDENTIFICADOR") {
        left = make_unique<VariableNode>(tokens[i].valor);
        pilaSemantica.push(left->getType(tablaSimbolos));
        i++;
    } 
    else {
        throw runtime_error("Factor inesperado: " + tokens[i].valor);
    }

    // Luego procesamos operadores
    while (i < tokens.size() && tokens[i].tipo == "ARITMETICO") {
        char op = tokens[i].valor[0];
        i++;
        
        unique_ptr<Node> right;
        if (tokens[i].tipo == "PARENTESIS_IZQUIERDO") {
            i++; // Saltar '('
            right = construirAST(tokens, i); // Recursión
            if (i >= tokens.size() || tokens[i].tipo != "PARENTESIS_DERECHO") {
                throw runtime_error("Se esperaba ')'");
            }
            i++; // Saltar ')'
        } 
        else if (tokens[i].tipo == "NUMERO") {
            right = make_unique<NumberNode>(tokens[i].valor);
            pilaSemantica.push(right->getType(tablaSimbolos));
            i++;
        } 
        else if (tokens[i].tipo == "IDENTIFICADOR") {
            right = make_unique<VariableNode>(tokens[i].valor);
            pilaSemantica.push(right->getType(tablaSimbolos));
            i++;
        } 
        else {
            throw runtime_error("Operando derecho inválido");
        }

        // Verificación de tipos
        DataType tipoDerecho = pilaSemantica.top(); pilaSemantica.pop();
        DataType tipoIzquierdo = pilaSemantica.top(); pilaSemantica.pop();
        
        if ((tipoIzquierdo == DataType::INT && tipoDerecho == DataType::FLOAT) ||
            (tipoIzquierdo == DataType::FLOAT && tipoDerecho == DataType::INT)) {
            pilaSemantica.push(DataType::FLOAT);
        } 
        else if (tipoIzquierdo == tipoDerecho) {
            pilaSemantica.push(tipoIzquierdo);
        } 
        else {
            throw runtime_error("Tipos incompatibles");
        }

        left = make_unique<OperatorNode>(op, move(left), move(right));
    }

    return left;
}

void mostrarExpresionSimple(const unique_ptr<Node>& node, const vector<Simbolo>& tablaSimbolos) {
    if (!node) return;
    
    if (auto numNode = dynamic_cast<NumberNode*>(node.get())) {
        cout << numNode->value << "[" << dataTypeToString(numNode->getType(tablaSimbolos)) << "]";
    }
    else if (auto varNode = dynamic_cast<VariableNode*>(node.get())) {
        cout << varNode->name << "[" << dataTypeToString(varNode->getType(tablaSimbolos)) << "]";
    }
    else if (auto opNode = dynamic_cast<OperatorNode*>(node.get())) {
        cout << "(";
        mostrarExpresionSimple(opNode->left, tablaSimbolos);
        cout << " " << opNode->op << " ";
        mostrarExpresionSimple(opNode->right, tablaSimbolos);
        cout << ")[" << dataTypeToString(opNode->getType(tablaSimbolos)) << "]";
    }
}

// Analiza si un conjunto de tokens forma una operación aritmética válida
bool esOperacionAritmetica(const vector<Token>& tokens, size_t& i, unique_ptr<Node>& root) {
    if (i + 3 < tokens.size() &&
        tokens[i].tipo == "IDENTIFICADOR" || tokens[i].tipo == "PARENTESIS_IZQUIERDO" &&
        tokens[i + 1].tipo == "OPERADOR" || tokens[i+1].tipo== "PARENTESIS_IZQUIERDO" || tokens[i+1].tipo== "PARENTESIS_DERECHO" && 
        tokens[i + 1].valor == "=") {
        
        // Obtener el tipo de la variable destino
        string nombreVariable = tokens[i].valor;
        Simbolo simboloDestino;
        if (!buscarVariable(nombreVariable, simboloDestino)) {
            cout << "Error: Variable '" << nombreVariable << "' no declarada" << endl;
            return false;
        }
        
        // Guardar posición inicial para restaurar en caso de error
        size_t posInicial = i;
        i += 2;
        
        try {
            // Construir árbol para evaluar la expresión
            root = construirAST(tokens, i);
            if (!root) {
                cout << "Error al construir árbol de expresión" << endl;
                i = posInicial;
                return false;
            }
            
            // Verificar compatibilidad de tipos
            DataType tipoExpresion = pilaSemantica.top();
            pilaSemantica.pop();
            DataType tipoDestino = stringToDataType(simboloDestino.tipo);
            
            if (tipoExpresion != tipoDestino) {
                cout << "Error de tipo: No se puede asignar " << dataTypeToString(tipoExpresion) 
                     << " a variable de tipo " << simboloDestino.tipo << endl;
                i = posInicial;
                return false;
            }
            
            // Verificar punto y coma final
            if (i < tokens.size() && tokens[i].tipo == "OPERADOR" && tokens[i].valor == ";") {
                i++;
                // Evaluar expresión y actualizar valor en tabla de símbolos
                double resultado = root->evaluate(tablaSimbolos);
                
                // Actualizar el valor en la tabla de símbolos
                for (auto& simbolo : tablaSimbolos) {
                    if (simbolo.nombre == nombreVariable) {
                        if (tipoDestino == DataType::INT) {
                            // Truncar a entero para tipo int
                            simbolo.valor = to_string(static_cast<int>(resultado));
                        } else {
                            simbolo.valor = to_string(resultado);
                        }
                        break;
                    }
                }
                
                // Muestra los datos de la expresion
                if (root) {
                    cout << "\nExpresión: ";
                    mostrarExpresionSimple(root, tablaSimbolos);
                    cout << "\nResultado: " << root->evaluate(tablaSimbolos) << endl;
                }
                cout << "--------------------------------------" << endl;

                
                return true;
            }
        } catch (const runtime_error& e) {
            cout << "Error en la operación: " << e.what() << endl;
            i = posInicial;
            return false;
        }
    }
    return false;
}

bool esLectura(const vector<Token>& tokens, size_t& i) {
    if (i + 3 < tokens.size() && 
        tokens[i].tipo == "LECTURA" &&
        tokens[i + 1].tipo == "PARENTESIS_IZQUIERDO" &&
        tokens[i + 2].tipo == "IDENTIFICADOR" &&
        tokens[i + 3].tipo == "PARENTESIS_DERECHO") {

        // Verificar si la variable está declarada
        Simbolo simbolo;
        if (!buscarVariable(tokens[i + 2].valor, simbolo)) {
            cout << "Error: Variable '" << tokens[i + 2].valor << "' no declarada" << endl;
            return false;
        }

        // Imprimir el valor de la variable
        cout << "El contenido de " << tokens[i + 2].valor << " es: " << simbolo.valor << endl;

        // Avanzar el índice después de procesar la lectura
        i += 4;

        // Verificar si hay punto y coma después
        if (i < tokens.size() && tokens[i].tipo == "OPERADOR" && tokens[i].valor == ";") {
            i++;
        }

        return true; // Retornar true si la instrucción es válida
    }
    return false;
}

bool esEscritura(const vector<Token>& tokens, size_t& i) {
    if (i + 3 < tokens.size() &&
        tokens[i].tipo == "ESCRITURA" &&
        tokens[i + 1].tipo == "PARENTESIS_IZQUIERDO" &&
        (tokens[i + 2].tipo == "IDENTIFICADOR" || tokens[i + 2].tipo == "NUMERO" || tokens[i + 2].tipo == "CADENA") &&
        tokens[i + 3].tipo == "PARENTESIS_DERECHO") {

        // Verificar si el valor a escribir es una variable declarada
        if (tokens[i + 2].tipo == "IDENTIFICADOR") {
            Simbolo simbolo;
            if (!buscarVariable(tokens[i + 2].valor, simbolo)) {
                cout << "Error: Variable '" << tokens[i + 2].valor << "' no declarada" << endl;
                return false;
            }
        }

        // Imprimir el valor a escribir
        cout << "Valor a escribir: " << tokens[i + 2].valor << endl;

        // Avanzar el índice después de procesar la escritura
        i += 4;

        // Verificar si hay punto y coma después
        if (i < tokens.size() && tokens[i].tipo == "OPERADOR" && tokens[i].valor == ";") {
            i++;
        }

        return true;
    }
    return false;
}

// Analiza si un conjunto de tokens forma una declaración de variable válida
bool esDeclaracionVariable(const vector<Token>& tokens, size_t& i) {
    if (i + 3 < tokens.size() &&
        tokens[i].tipo == "VARIABLE" &&
        tokens[i + 1].tipo == "IDENTIFICADOR" &&
        tokens[i + 2].tipo == "OPERADOR" && tokens[i + 2].valor == "=") {

        string tipo = tokens[i].valor; // "int", "float" o "string"
        string nombre = tokens[i + 1].valor; // Nombre de la variable
        string valor;

        // Verificar si ya existe la variable
        for (const auto& simbolo : tablaSimbolos) {
            if (simbolo.nombre == nombre) {
                cout << "Error: Variable '" << nombre << "' ya declarada" << endl;
                return false;
            }
        }

        // Obtener el valor de la inicialización
        if (tokens[i + 3].tipo == "NUMERO" || tokens[i + 3].tipo == "CADENA") {
            valor = tokens[i + 3].valor;
        } else if (tokens[i + 3].tipo == "IDENTIFICADOR") {
            // Es un identificador (otra variable)
            string nombreVar = tokens[i + 3].valor;
            Simbolo varExistente;
            if (!buscarVariable(nombreVar, varExistente)) {
                cout << "Error: Variable '" << nombreVar << "' no declarada" << endl;
                return false;
            }

            // Verificar compatibilidad de tipos
            if (tipo != varExistente.tipo) {
                cout << "Error de tipo: No se puede asignar " << varExistente.tipo 
                     << " a variable de tipo " << tipo << endl;
                return false;
            }

            valor = varExistente.valor;
        } else {
            cout << "Error: Valor no válido en la declaración de la variable" << endl;
            return false;
        }

        // Verificar que el valor sea compatible con el tipo
        if (!esValorCompatible(valor, tipo)) {
            cout << "Error: El valor '" << valor << "' no es compatible con el tipo '" << tipo << "'" << endl;
            return false;
        }

        // Agregar variable a la tabla de símbolos
        string idContador = "id" + to_string(idGlobalCounter++);
        tablaSimbolos.push_back({nombre, tipo, valor, idContador});

        i += 4;

        // Verificar si hay punto y coma después
        if (i < tokens.size() && tokens[i].tipo == "OPERADOR" && tokens[i].valor == ";") {
            i++;
            return true;
        }
    }
    return false;
}

// Declaraciones adelantadas para funciones recursivas
bool esBloqueIf(const vector<Token>& tokens, size_t& i);
bool esBloqueWhile(const vector<Token>& tokens, size_t& i);

// Analiza un bloque de código entre llaves
bool analizarBloque(const vector<Token>& tokens, size_t& i) {
    unique_ptr<Node> root;
    while (i < tokens.size() && tokens[i].tipo != "LLAVE_DERECHA") {
        // Intentar reconocer diferentes construcciones del lenguaje
        if (esDeclaracionVariable(tokens, i)) continue;
        if (esEscritura(tokens, i)) continue;
        if (esLectura(tokens, i)) continue;
        if (esBloqueIf(tokens, i)) continue;
        if (esBloqueWhile(tokens, i)) continue;
        if (esOperacionAritmetica(tokens, i, root)) continue;
        return false;
    }
    return true;
}

// Analiza si un conjunto de tokens forma un bucle while válido (solo con int)
bool esBloqueWhile(const vector<Token>& tokens, size_t& i) {
    if (i + 6 < tokens.size() &&
        tokens[i].tipo == "CICLO" && tokens[i].valor == "while" &&
        tokens[i + 1].tipo == "PARENTESIS_IZQUIERDO" &&
        (tokens[i + 2].tipo == "IDENTIFICADOR" || tokens[i + 2].tipo == "NUMERO") &&
        tokens[i + 3].tipo == "COMPARACION" &&
        (tokens[i + 4].tipo == "IDENTIFICADOR" || tokens[i + 4].tipo == "NUMERO") &&
        tokens[i + 5].tipo == "PARENTESIS_DERECHO") {
        
        i += 6;
        
        // Verificar bloque de código
        if (i < tokens.size() && tokens[i].tipo == "LLAVE_IZQUIERDA") {
            i++;
            if (!analizarBloque(tokens, i)) {
                return false;
            }
            if (i < tokens.size() && tokens[i].tipo == "LLAVE_DERECHA") {
                i++;
                return true;
            }
        }
    }
    return false;
}


bool esBloqueIf(const vector<Token>& tokens, size_t& i) {
    if (i + 6 < tokens.size() &&
        tokens[i].tipo == "CONDICION" && tokens[i].valor == "if" &&
        tokens[i + 1].tipo == "PARENTESIS_IZQUIERDO" &&
        (tokens[i + 2].tipo == "IDENTIFICADOR" || tokens[i + 2].tipo == "NUMERO") &&
        tokens[i + 3].tipo == "COMPARACION" &&
        (tokens[i + 4].tipo == "IDENTIFICADOR" || tokens[i + 4].tipo == "NUMERO") &&
        tokens[i + 5].tipo == "PARENTESIS_DERECHO") {
        
        // Verificación de tipos
        DataType tipoIzq, tipoDer;
        
        // Determinar tipo del operando izquierdo
        if (tokens[i + 2].tipo == "NUMERO") {
            tipoIzq = tokens[i + 2].valor.find('.') != string::npos ? DataType::FLOAT : DataType::INT;
        } else {
            Simbolo simboloIzq;
            if (!buscarVariable(tokens[i + 2].valor, simboloIzq)) {
                cout << "Error: Variable '" << tokens[i + 2].valor << "' no declarada" << endl;
                return false;
            }
            tipoIzq = stringToDataType(simboloIzq.tipo);
        }
        
        // Determinar tipo del operando derecho
        if (tokens[i + 4].tipo == "NUMERO") {
            tipoDer = tokens[i + 4].valor.find('.') != string::npos ? DataType::FLOAT : DataType::INT;
        } else {
            Simbolo simboloDer;
            if (!buscarVariable(tokens[i + 4].valor, simboloDer)) {
                cout << "Error: Variable '" << tokens[i + 4].valor << "' no declarada" << endl;
                return false;
            }
            tipoDer = stringToDataType(simboloDer.tipo);
        }
        
        // Avanzar después de la condición
        i += 6;
        
        // Verificar bloque de código del if
        if (i < tokens.size() && tokens[i].tipo == "LLAVE_IZQUIERDA") {
            i++;
            if (!analizarBloque(tokens, i)) {
                return false;
            }
            if (i < tokens.size() && tokens[i].tipo == "LLAVE_DERECHA") {
                i++;
                
                // Verificar si hay un else después
                if (i < tokens.size() && tokens[i].tipo == "CONDICION" && tokens[i].valor == "else") {
                    i++;
                    
                    // Verificar bloque de código del else
                    if (i < tokens.size() && tokens[i].tipo == "LLAVE_IZQUIERDA") {
                        i++;
                        if (!analizarBloque(tokens, i)) {
                            return false;
                        }
                        if (i < tokens.size() && tokens[i].tipo == "LLAVE_DERECHA") {
                            i++;
                            return true;
                        }
                    } else {
                        cout << "Error: Se esperaba '{' después del else" << endl;
                        return false;
                    }
                }
                return true; // Tanto if con else como if sin else son válidos
            }
        }
    }
    return false;
}

// Función principal para el análisis sintáctico del código
bool analisisSintactico(const vector<Token>& tokens) {
    size_t i = 0;
    unique_ptr<Node> root;
    while (i < tokens.size()) {
        limpiarPilaSemantica();  // Limpiar la pila antes de analizar una nueva expresión
        // Intentar reconocer diferentes construcciones del lenguaje
        if (esDeclaracionVariable(tokens, i)) continue;
        if (esEscritura(tokens, i)) continue;
        if (esLectura(tokens, i)) continue;
        if (esBloqueIf(tokens, i)) continue;
        if (esBloqueWhile(tokens, i)) continue;
        if (esOperacionAritmetica(tokens, i, root)) continue;
        return false;
    }
    return true;
}

void limpiarPilaSemantica() {
    while (!pilaSemantica.empty()) {
        pilaSemantica.pop();
    }
}

// Función principal del programa
int main() {
    // Abrir archivo con código fuente
    string file_name = "codigo.txt";
    ifstream archivo(file_name);
    if (!archivo) {
        cout << "Error al encontrar el archivo " << file_name << "\n";
        return 1;
    }
    
    // Leer código fuente
    string linea, codigo_completo;
    while (getline(archivo, linea)) {
        if (!linea.empty()) {
            codigo_completo += linea + " ";
        }
    }
    archivo.close();
    
    // Realizar análisis léxico para obtener tokens
    vector<Token> tokens = Lexico(codigo_completo);
    
    // Mostrar tabla de tokens
    cout << "-------------------------------------\n";
    cout << "|         TABLA DE SIMBOLOS         |\n";
    cout << "-------------------------------------\n";
    for (const auto& token : tokens) {
        cout << left << setw(20) << token.tipo << "|  " << setw(15) << token.valor << endl;
    }
    
    // Realizar análisis sintáctico
    bool valido = analisisSintactico(tokens);
    cout << "\nCódigo válido: " << (valido ? "Sí" : "No") << "\n";
    
    // Mostrar tabla de símbolos (variables)
    cout << "\n---------------------------------------------------------\n";
    cout << "|           TABLA DE SÍMBOLOS - IDENTIFICADORES           |\n";
    cout << "---------------------------------------------------------\n";
    cout << "| Nombre        | Tipo       | Valor     | ID          |\n";
    cout << "---------------------------------------------------------\n";
    for (const auto& simbolo : tablaSimbolos) {
        cout << "| " << setw(12) << simbolo.nombre << " | "
             << setw(10) << simbolo.tipo << " | "
             << setw(10) << simbolo.valor << " | "
             << setw(10) << simbolo.id_contador << " |" << endl;
    }
    cout << "---------------------------------------------------------\n";
    return 0;
}