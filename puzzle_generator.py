import os
import random
import subprocess
import re
import logging
import argparse
from itertools import combinations


# Configurable Constants
SOLVER_PATH = "./solver"
SOLUTIONS_DIR = "solutions"
INIT_PIECE_COUNT = 9
MIN_PIECES_TO_SELECT = 2.3
MAX_PIECES_TO_SELECT = 11.0
MIN_LEVEL_FOR_ONE_PIECE_PUZZLE = 17

class PuzzleGenerator:
    def __init__(self, solver_path, solutions_dir):
        self.solver_path = solver_path
        self.solutions_dir = solutions_dir
        self.input_file = "board.tmp"
        self.logger = logging.getLogger(__name__)
    
    def validate_input_directory(self, directory):
        if not os.path.exists(directory) or not os.path.isdir(directory):
            self.logger.error(f"Directory '{directory}' does not exist or is not a directory.")
            return False
        return True

    def validate_input_executable(self, path):
        if not os.path.exists(path) or not os.path.isfile(path):
            self.logger.error("Solver executable not found.")
            return False
        return True
    
    def create_input_file(self, board):
        self.logger.debug("Creating input file with board configuration.")
        with open(self.input_file, 'w') as file:
            file.write(board)

    def run_external_program(self, program_path, *args):
        self.logger.debug(f"Running external program: {self.solver_path} {' '.join(args)}")
        try:
            command = [program_path, *args]
            result = subprocess.run(command, text=True, capture_output=True, check=True)
            return result.stdout, result.stderr
        except subprocess.CalledProcessError as e:
            return f"Error: {e}", e.stderr

    def generate_board_combinations(self, file_contents, num_pieces_to_select):
        self.logger.debug(f"Generating board combinations with {num_pieces_to_select} pieces.")
        selected_combinations = []
        for content in file_contents:
            if len(content) >= num_pieces_to_select:
                row_combinations = combinations(content, round(num_pieces_to_select))
                selected_combinations.extend(row_combinations)
        return selected_combinations

    def adjust_difficulty(self, moves, low, high, num_pieces_to_select):
        if moves < low:
            num_pieces_to_select -= 0.1
        elif moves > high:
            num_pieces_to_select += 0.1
        return max(MIN_PIECES_TO_SELECT, min(MAX_PIECES_TO_SELECT, num_pieces_to_select))

    def extract_moves(self, solver_output):
        moves_match = re.search(r'using (\d+) moves', solver_output)
        if moves_match:
           return int(moves_match.group(1))
        else:
            print("Couldn't find moves in solver output:")
            print(solver_output)
            exit()

    def calculate_ranges(self, levels):
        self.logger.debug("Calculating difficulty ranges.")
        base_low = 7.0
        base_high = 20.0
        ranges = []

        for i in range(levels):
            low = base_low if i == 0 else base_high
            high = base_high + low/2.2
            ranges.append((low, high))
            base_high = high

        return ranges

    def get_level(self):
        self.logger.debug(f"Getting user input for puzzle level (1-20).")
        while True:
            try:
                level = int(input("Enter level (1-20): "))
                if 1 <= level <= 20:
                    return level
                else:
                    print("Please enter a level between 1 and 20.")
            except ValueError:
                print("Invalid input. Please enter a number.")

    def unexpected_error(self, solver_output, solver_error):
        self.logger.error("An unexpected error has occurred.")
        self.logger.debug("Solver output:\n%s", solver_output)
        self.logger.debug("Solver error output:\n%s", solver_error)
        print("An unexpected error has occurred. Please check logs for details.")
        exit(1)

    def get_solutions(self, directory):
        self.logger.debug(f"Getting solutions from directory: {self.solutions_dir}")
        file_contents = []

        if os.path.exists(directory) and os.path.isdir(directory):
            files = os.listdir(directory)
            for filename in files:
                file_path = os.path.join(directory, filename)
                if os.path.isfile(file_path):
                    with open(file_path, 'r') as file:
                        content = file.read().splitlines()
                        file_contents.append(content)
        else:
            print(f"Directory '{directory}' does not exist or is not a directory.")
            exit()

        return file_contents
    
    def calculate_initial_piece_count(self, level):
        return INIT_PIECE_COUNT - level/3

    def extract_solution_count(self, solver_output, solver_error):
        solution_match = re.search(r'Found (\d+) solution', solver_output)

        if not solution_match:
            self.unexpected_error(solver_output, solver_error)
        
        return int(solution_match.group(1))
        
    def generate_one_piece_puzzle(self, low, high):
        self.logger.info(f"Generating a one-piece puzzle with difficulty range: {low}-{high}")

        with open("1p1s_combinations", 'r') as file:
            content = file.read().splitlines()
        
        random.shuffle(content)
        
        for piece in content:
            self.create_input_file(piece)
            solver_output, solver_error = self.run_external_program(self.solver_path, self.input_file)

            solutions = self.extract_solution_count(solver_output, solver_error)

            if solutions < 1:
                self.logger.info(f"No solution found")
                continue
            
            moves = self.extract_moves(solver_output)
            self.logger.info(f"Using {moves} moves")

            if low <= moves <= high and solutions == 1:
                return solver_output
            else:
                self.logger.info("Puzzle does not meet criteria. Trying again...")
            
    def generate_multi_piece_puzzle(self, all_solutions, level, low, high):
        self.logger.info(f"Generating puzzle for level {level} with difficulty range: {low}-{high}")
        num_pieces_to_select = self.calculate_initial_piece_count(level)
        
        while True:
            self.logger.info("Use " + str(round(num_pieces_to_select)) + " pieces...")
            random.shuffle(all_solutions)
            selected_combinations = self.generate_board_combinations(all_solutions, num_pieces_to_select)
            combo = selected_combinations[0]
            board = "\n".join(combo)
            self.create_input_file(board)

            solver_output, solver_error = self.run_external_program(self.solver_path, self.input_file)
            solutions = self.extract_solution_count(solver_output, solver_error)
            moves = self.extract_moves(solver_output)

            if solutions < 1:
                self.logger.info(f"No solution found...")
                continue
            
            self.logger.info(f"Using {moves} moves...")

            if moves < low:
                self.logger.info("Making puzzle harder...")
            elif moves > high:
                self.logger.info("Making puzzle easier...")
            else:
                if solutions == 1:
                    return solver_output
                else:
                    self.logger.info("No unique solution, trying again...")
            
            num_pieces_to_select = self.adjust_difficulty(moves, low, high, num_pieces_to_select)

    def display_solution(self, solver_output):
        pattern = r'solution:((.|\n)*)'
        result = re.search(pattern, solver_output)
        if result:
            solution = result.group(1)
            print("Solution:")
            print(solution)
        else:
            print("Error: No solution found.")


    def display_puzzle(self, solver_output):
        pattern = r'X((.|\n)*?)S'
        result = re.search(pattern, solver_output)
        if result:
            puzzle = result.group(1)
            print("Found puzzle!")
            print(puzzle)
        else:
            print("Error: No puzzle found.")



def main():
    logging.basicConfig(level=logging.INFO)

    parser = argparse.ArgumentParser(description="Generate puzzles with varying levels of difficulty.")
    parser.add_argument("--solver", default="./solver", help="Path to the solver executable (default: ./solver)")
    parser.add_argument("--solutions-dir", default="solutions", help="Directory containing solution files (default: solutions)")
    args = parser.parse_args()

    generator = PuzzleGenerator(args.solver, args.solutions_dir)

    if not generator.validate_input_executable(args.solver):
        return

    if not generator.validate_input_directory(args.solutions_dir):
        return

    all_solutions = generator.get_solutions(args.solutions_dir)

    ranges = generator.calculate_ranges(20)
    level = generator.get_level()

    low, high = [round(x) for x in ranges[level - 1]]

    if level >= MIN_LEVEL_FOR_ONE_PIECE_PUZZLE:
        solver_output = generator.generate_one_piece_puzzle(low, high)
    else:
        solver_output = generator.generate_multi_piece_puzzle(all_solutions, level, low, high)
    
    generator.display_puzzle(solver_output)
    input("Press Enter to display solution...")
    generator.display_solution(solver_output)


if __name__ == "__main__":
    main()