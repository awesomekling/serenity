/*
 * Copyright (c) 2020-2021, Denis Campredon <deni_@hotmail.fr>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "I386Assembly.h"
#include <AK/SourceGenerator.h>
#include <LibCpp/LibIntermediate/SIR.h>
#include <LibCpp/Option.h>

namespace BackEnd {

NonnullRefPtr<Core::File> I386Assembly::get_output_file()
{
    auto output_file = Core::File::open(m_options.output_file, Core::IODevice::WriteOnly);
    assert(!output_file.is_error());

    return output_file.value();
}

String I386Assembly::get_register_for_expression(const HashMap<String, String>& variables_already_seen, const RefPtr<SIR::Expression>& expression)
{
    if (expression->is_identifier_expression() || expression->is_binary_expression()) {
        auto result = variables_already_seen.get(expression->result()->name());
        ASSERT(result.has_value());
        return result.value();
    } else {
        ASSERT(expression->is_constant_expression());
        return String::format("$%i", reinterpret_cast<const AK::NonnullRefPtr<SIR::ConstantExpression>&>(expression)->value());
    }
}

void I386Assembly::print_assembly_for_function(const SIR::Function& function)
{
    size_t param_stack = m_param_stack_start;
    StringBuilder builder;
    auto generator = SourceGenerator(builder, '{', '}');
    generator.set("function.name", function.name());

    generator.append("\t.globl {function.name}\n");
    generator.append("\t.type {function.name}, @function\n");
    generator.append("{function.name}:\n");
    generator.append("\tpushl\t%ebp\n");
    generator.append("\tmovl\t%esp, %ebp\n");

    auto variables_already_seen = HashMap<String, String>();
    Optional<const SIR::Variable*> var_in_eax;

    for (auto& operation : function.body()) {
        if (operation.is_binary_expression()) {
            auto& binop = reinterpret_cast<const SIR::BinaryExpression&>(operation);
            auto& right = binop.right();
            auto& left = binop.left();
            String right_value = get_register_for_expression(variables_already_seen, right);
            String left_value = get_register_for_expression(variables_already_seen, left);
            auto left_index = variables_already_seen.get(left->result()->name());

            generator.set("right_operand.index", right_value);
            generator.set("left_operand.index", left_value);

            if (!var_in_eax.has_value() || var_in_eax.value() != binop.left()->result().ptr())
                generator.append("\tmovl\t{left_operand.index}, %eax\n");

            switch (binop.binary_operation()) {
            case SIR::BinaryExpression::Kind::Addition:
                generator.append("\taddl\t{right_operand.index}, %eax\n");
                break;
            case SIR::BinaryExpression::Kind::Multiplication:
                generator.append("\timull\t{right_operand.index}, %eax\n");
                break;
            case SIR::BinaryExpression::Kind::Subtraction:
                generator.append("\tsubl\t{right_operand.index}, %eax\n");
                break;
            case SIR::BinaryExpression::Kind::And:
                generator.append("\tandl\t{right_operand.index}, %eax\n");
                break;
            case SIR::BinaryExpression::Kind::Xor:
                generator.append("\txorl\t{right_operand.index}, %eax\n");
                break;
            case SIR::BinaryExpression::Kind::Or:
                generator.append("\torl\t{right_operand.index}, %eax\n");
                break;
            case SIR::BinaryExpression::Kind::Division:
                generator.append("\tcltd\n");
                generator.append("\tidivl\t{right_operand.index}\n");
                break;
            case SIR::BinaryExpression::Kind::Modulo:
                generator.append("\tcltd\n");
                generator.append("\tidivl\t{right_operand.index}\n");
                generator.append("\tmovl\t%edx, %eax\n");
                break;
            case SIR::BinaryExpression::Kind::LeftShift:
                generator.append("\tmovl\t{right_operand.index}, %ecx\n");
                generator.append("\tshll\t%cl, %eax\n");
                break;
            case SIR::BinaryExpression::Kind::RightShift:
                generator.append("\tmovl\t{right_operand.index}, %ecx\n");
                generator.append("\tsarl\t%cl, %eax\n");
                break;
            case SIR::BinaryExpression::Kind::NotEqual:
                generator.append("\tcmpl\t{right_operand.index}, %eax\n");
                break;
            }
            //TODO: clear other var in eax, and check that there are no var in ecx and edx
            variables_already_seen.set(binop.result()->name(), "%eax");
            var_in_eax = binop.result();
        } else if (operation.is_return_statement()) {
            auto& stmt = reinterpret_cast<const SIR::ReturnStatement&>(operation);

            if (stmt.expression() && !var_in_eax.has_value()) {
                generator.set("operand.stack_position", get_register_for_expression(variables_already_seen, stmt.expression()));
                generator.append("\tmovl\t{operand.stack_position}, %eax\n");
            }
            generator.append("\tpopl\t%ebp\n\tret\n");
        } else if (operation.is_variable()) {
            auto& var = reinterpret_cast<const SIR::Variable&>(operation);
            generator.set("operand.stack_position", String::format("%zu", param_stack));
            generator.append("\tmovl\t{operand.stack_position}(%ebp), %eax\n");
            variables_already_seen.set(var.name(), String::format("%zu(%%ebp)", param_stack));
            param_stack += var.node_type()->size_in_bytes();
            var_in_eax = &var;
        } else if (operation.is_label_expression()) {
            auto& label = reinterpret_cast<const SIR::LabelExpression&>(operation);
            generator.set("label.identifier", label.identifier());

            generator.append("{label.identifier}:\n");
        } else if (operation.is_jump_statement()) {
            dbgln("{}", generator.as_string());
            auto& jump = reinterpret_cast<const SIR::JumpStatement&>(operation);
            auto& if_true = jump.if_true().at(0);
            auto& if_false = jump.if_false();
            ASSERT(if_true.is_expression() && if_false->is_expression());
            generator.set("if.identifier", reinterpret_cast<const SIR::Expression&>(if_true).result()->name());
            generator.set("else.identifier", if_false->result()->name());

            generator.append("\tje\t{if.identifier}\n");
            generator.append("\tjmp\t{else.identifier}\n");
            var_in_eax.clear();
        } else {
            ASSERT_NOT_REACHED();
        }
    }

    generator.append("\t.size {function.name}, .-{function.name}\n");
    m_output_file->write(generator.as_string());
}

void I386Assembly::print_asm()
{
    //TODO: implement String.last_index_of
    auto input_file_name = m_options.input_file;
    while (input_file_name.contains("/")) {
        auto new_path = input_file_name.index_of("/");

        assert(new_path.has_value());
        input_file_name = input_file_name.substring(new_path.value() + 1);
    }

    StringBuilder builder;
    auto generator = SourceGenerator(builder, '{', '}');
    generator.set("input.filename", input_file_name);
    generator.append("\t.file \"{input.filename}\"\n");
    generator.append("\t.ident \"Serenity-c++ compiler V0.0.0\"\n");
    generator.append("\t.section \".note.GNU-stack\",\"\",@progbits\n");

    m_output_file->write(generator.as_string());

    for (auto& function : m_tu.functions()) {
        print_assembly_for_function(function);
    }
}

}
