#ifndef VVIM_H
#define VVIM_H

#include <QObject>
#include <QString>
#include <QTextCursor>
#include <QMap>
#include <QDebug>
#include "vutils.h"

class VEdit;
class QKeyEvent;
class VEditConfig;
class QKeyEvent;

enum class VimMode {
    Normal = 0,
    Insert,
    Visual,
    VisualLine,
    Replace,
    Invalid
};

class VVim : public QObject
{
    Q_OBJECT
public:
    explicit VVim(VEdit *p_editor);

    struct Register
    {
        Register(QChar p_name, const QString &p_value)
            : m_name(p_name), m_value(p_value), m_append(false)
        {
        }

        Register(QChar p_name)
            : m_name(p_name), m_append(false)
        {
        }

        Register()
            : m_append(false)
        {
        }

        // Register a-z.
        bool isNamedRegister() const
        {
            char ch = m_name.toLatin1();
            return ch >= 'a' && ch <= 'z';
        }

        bool isUnnamedRegister() const
        {
            return m_name == c_unnamedRegister;
        }

        bool isBlackHoleRegister() const
        {
            return m_name == c_blackHoleRegister;
        }

        bool isSelectionRegister() const
        {
            return m_name == c_selectionRegister;
        }

        bool isBlock() const
        {
            return m_value.endsWith('\n');
        }

        // @p_value is the content to update.
        // If @p_value ends with \n, then it is a block.
        // When @p_value is a block, we need to add \n at the end if necessary.
        // If @m_append is true and @p_value is a block, we need to add \n between
        // them if necessary.
        void update(const QString &p_value);

        // Read the value of this register.
        const QString &read();

        QChar m_name;
        QString m_value;

        // This is not info of Register itself, but a hint to the handling logics
        // whether we need to append the content to this register.
        // Only valid for a-z registers.
        bool m_append;
    };

    // Handle key press event.
    // Returns true if the event is consumed and need no more handling.
    bool handleKeyPressEvent(QKeyEvent *p_event);

    // Return current mode.
    VimMode getMode() const;

    // Set current mode.
    void setMode(VimMode p_mode);

    // Set current register.
    void setRegister(QChar p_reg);

    // Get m_registers.
    const QMap<QChar, Register> &getRegisters() const;

    QChar getCurrentRegisterName() const;

    // Get pending keys.
    // Turn m_pendingKeys to a string.
    QString getPendingKeys() const;

signals:
    // Emit when current mode has been changed.
    void modeChanged(VimMode p_mode);

    // Emit when VVim want to display some messages.
    void vimMessage(const QString &p_msg);

    // Emit when current status updated.
    void vimStatusUpdated(const VVim *p_vim);

private slots:
    // When user use mouse to select texts in Normal mode, we should change to
    // Visual mode.
    void selectionToVisualMode(bool p_hasText);

private:
    // Struct for a key press.
    struct Key
    {
        Key(int p_key, int p_modifiers = Qt::NoModifier)
            : m_key(p_key), m_modifiers(p_modifiers)
        {
        }

        Key() : m_key(-1), m_modifiers(Qt::NoModifier) {}

        int m_key;
        int m_modifiers;

        bool isDigit() const
        {
            return m_key >= Qt::Key_0
                   && m_key <= Qt::Key_9
                   && m_modifiers == Qt::NoModifier;
        }

        int toDigit() const
        {
            V_ASSERT(isDigit());
            return m_key - Qt::Key_0;
        }

        bool isAlphabet() const
        {
            return m_key >= Qt::Key_A
                   && m_key <= Qt::Key_Z
                   && (m_modifiers == Qt::NoModifier || m_modifiers == Qt::ShiftModifier);
        }

        QChar toAlphabet() const
        {
            V_ASSERT(isAlphabet());
            if (m_modifiers == Qt::NoModifier) {
                return QChar('a' + (m_key - Qt::Key_A));
            } else {
                return QChar('A' + (m_key - Qt::Key_A));
            }
        }

        bool isValid() const
        {
            return m_key > -1 && m_modifiers > -1;
        }

        bool operator==(const Key &p_key) const
        {
            return p_key.m_key == m_key && p_key.m_modifiers == m_modifiers;
        }
    };

    // Supported actions.
    enum class Action
    {
        Move = 0,
        Delete,
        Copy,
        Paste,
        PasteBefore,
        Change,
        Indent,
        UnIndent,
        ToUpper,
        ToLower,
        Invalid
    };

    // Supported movements.
    enum class Movement
    {
        Left = 0,
        Right,
        Up,
        Down,
        VisualUp,
        VisualDown,
        PageUp,
        PageDown,
        HalfPageUp,
        HalfPageDown,
        StartOfLine,
        EndOfLine,
        FirstCharacter,
        LineJump,
        StartOfDocument,
        EndOfDocument,
        WordForward,
        WORDForward,
        ForwardEndOfWord,
        ForwardEndOfWORD,
        WordBackward,
        WORDBackward,
        BackwardEndOfWord,
        BackwardEndOfWORD,
        FindForward,
        FindBackward,
        TillForward,
        TillBackward,
        Invalid
    };

    // Supported ranges.
    enum class Range
    {
        Line = 0,
        WordInner,
        WordAround,
        WORDInner,
        WORDAround,
        QuoteInner,
        QuoteAround,
        DoubleQuoteInner,
        DoubleQuoteAround,
        ParenthesisInner,
        ParenthesisAround,
        BracketInner,
        BracketAround,
        AngleBracketInner,
        AngleBracketAround,
        BraceInner,
        BraceAround,
        Invalid
    };

    enum class TokenType { Action = 0, Repeat, Movement, Range, Invalid };

    struct Token
    {
        Token(Action p_action)
            : m_type(TokenType::Action), m_action(p_action) {}

        Token(int p_repeat)
            : m_type(TokenType::Repeat), m_repeat(p_repeat) {}

        Token(Movement p_movement)
            : m_type(TokenType::Movement), m_movement(p_movement) {}

        Token(Movement p_movement, Key p_key)
            : m_type(TokenType::Movement), m_movement(p_movement), m_key(p_key) {}

        Token(Range p_range)
            : m_type(TokenType::Range), m_range(p_range) {}

        Token() : m_type(TokenType::Invalid) {}

        bool isRepeat() const
        {
            return m_type == TokenType::Repeat;
        }

        bool isAction() const
        {
            return m_type == TokenType::Action;
        }

        bool isMovement() const
        {
            return m_type == TokenType::Movement;
        }

        bool isRange() const
        {
            return m_type == TokenType::Range;
        }

        bool isValid() const
        {
            return m_type != TokenType::Invalid;
        }

        QString toString() const
        {
            QString str;
            switch (m_type) {
            case TokenType::Action:
                str = QString("action %1").arg((int)m_action);
                break;

            case TokenType::Repeat:
                str = QString("repeat %1").arg(m_repeat);
                break;

            case TokenType::Movement:
                str = QString("movement %1").arg((int)m_movement);
                break;

            case TokenType::Range:
                str = QString("range %1").arg((int)m_range);
                break;

            default:
                str = "invalid";
            }

            return str;
        }

        TokenType m_type;

        union
        {
            Action m_action;
            int m_repeat;
            Range m_range;
            Movement m_movement;
        };

        // Used in some Movement.
        Key m_key;
    };

    // Reset all key state info.
    void resetState();

    // Now m_tokens constitute a command. Execute it.
    // Will clear @p_tokens.
    void processCommand(QList<Token> &p_tokens);

    // Return the number represented by @p_keys.
    // Return -1 if @p_keys is not a valid digit sequence.
    int numberFromKeySequence(const QList<Key> &p_keys);

    // Try to generate a Repeat token from @p_keys and insert it to @p_tokens.
    // If succeed, clear @p_keys and return true.
    bool tryGetRepeatToken(QList<Key> &p_keys, QList<Token> &p_tokens);

    // @p_tokens is the arguments of the Action::Move action.
    void processMoveAction(QList<Token> &p_tokens);

    // @p_tokens is the arguments of the Action::Delete action.
    void processDeleteAction(QList<Token> &p_tokens);

    // @p_tokens is the arguments of the Action::Copy action.
    void processCopyAction(QList<Token> &p_tokens);

    // @p_tokens is the arguments of the Action::Paste and Action::PasteBefore action.
    void processPasteAction(QList<Token> &p_tokens, bool p_pasteBefore);

    // @p_tokens is the arguments of the Action::Change action.
    void processChangeAction(QList<Token> &p_tokens);

    // @p_tokens is the arguments of the Action::Indent and Action::UnIndent action.
    void processIndentAction(QList<Token> &p_tokens, bool p_isIndent);

    // @p_tokens is the arguments of the Action::ToLower and Action::ToUpper action.
    void processToLowerAction(QList<Token> &p_tokens, bool p_toLower);

    // Clear selection if there is any.
    // Returns true if there is selection.
    bool clearSelection();

    // Get the block count of one page step in vertical scroll bar.
    int blockCountOfPageStep() const;

    // Expand selection to whole lines which will change the position
    // of @p_cursor.
    void expandSelectionToWholeLines(QTextCursor &p_cursor);

    // Init m_registers.
    // Currently supported registers:
    // a-z, A-Z (append to a-z), ", +, _
    void initRegisters();

    // Check m_keys to see if we are expecting a register name.
    bool expectingRegisterName() const;

    // Check m_keys to see if we are expecting a target for f/t/F/T command.
    bool expectingCharacterTarget() const;

    // Return the corresponding register name of @p_key.
    // If @p_key is not a valid register name, return a NULL QChar.
    QChar keyToRegisterName(const Key &p_key) const;

    // Check if @m_tokens contains an action token.
    bool hasActionToken() const;

    // Try to add an Action::Move action at the front if there is no any action
    // token.
    void tryAddMoveAction();

    // Add an Action token in front of m_tokens.
    void addActionToken(Action p_action);

    // Get the action token from m_tokens.
    const Token *getActionToken() const;

    // Add an Range token at the end of m_tokens.
    void addRangeToken(Range p_range);

    // Add an Movement token at the end of m_tokens.
    void addMovementToken(Movement p_movement);

    // Add an Movement token at the end of m_tokens.
    void addMovementToken(Movement p_movement, Key p_key);

    // Delete selected text if there is any.
    // @p_clearEmptyBlock: whether to remove the empty block after deletion.
    void deleteSelectedText(QTextCursor &p_cursor, bool p_clearEmptyBlock);

    // Copy selected text if there is any.
    // Will clear selection.
    // @p_addNewLine: whether to add a new line \n to the selection.
    void copySelectedText(bool p_addNewLine);

    void copySelectedText(QTextCursor &p_cursor, bool p_addNewLine);

    // Convert the case of selected text if there is any.
    // Will clear selection.
    // @p_toLower: to lower or upper.
    void convertCaseOfSelectedText(QTextCursor &p_cursor, bool p_toLower);

    // Save @p_text to the Register pointed by m_regName.
    // Remove QChar::ObjectReplacementCharacter before saving.
    void saveToRegister(const QString &p_text);

    // Move @p_cursor according to @p_moveMode and @p_token.
    // Return true if it has moved @p_cursor.
    bool processMovement(QTextCursor &p_cursor, const QTextDocument *p_doc,
                         QTextCursor::MoveMode p_moveMode,
                         const Token &p_token, int p_repeat);

    // Move @p_cursor according to @p_moveMode and @p_range.
    // Return true if it has moved @p_cursor.
    bool selectRange(QTextCursor &p_cursor, const QTextDocument *p_doc,
                     Range p_range, int p_repeat);

    // Check if there is an Action token with Delete/Copy/Change action.
    bool hasActionTokenValidForTextObject() const;

    // Check if m_keys only contains @p_key.
    bool checkPendingKey(const Key &p_key) const;

    // Check if m_tokens only contains action token @p_action.
    bool checkActionToken(Action p_action) const;

    // Repeat m_lastFindToken.
    void repeatLastFindMovement(bool p_reverse);

    void message(const QString &p_str);

    VEdit *m_editor;
    const VEditConfig *m_editConfig;
    VimMode m_mode;

    // A valid command token should follow the rule:
    // Action, Repeat, Movement.
    // Action, Repeat, Range.
    // Action, Repeat.
    QList<Key> m_keys;
    QList<Token> m_tokens;

    // Keys for status indication.
    QList<Key> m_pendingKeys;

    // Whether reset the position in block when moving cursor.
    bool m_resetPositionInBlock;

    QMap<QChar, Register> m_registers;

    // Currently used register.
    QChar m_regName;

    // Last f/F/t/T Token.
    Token m_lastFindToken;

    static const QChar c_unnamedRegister;
    static const QChar c_blackHoleRegister;
    static const QChar c_selectionRegister;
};

#endif // VVIM_H