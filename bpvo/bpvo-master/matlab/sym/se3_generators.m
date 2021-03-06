function V = se3_generators


  V{4} = [0 0 0 1; ...
          0 0 0 0; ...
          0 0 0 0; ...
          0 0 0 0];

  V{5} = [0 0 0 0; ...
          0 0 0 1; ...
          0 0 0 0; ...
          0 0 0 0];

  V{6} = [0 0 0 0; ...
          0 0 0 0; ...
          0 0 0 1; ...
          0 0 0 0];

  V{1} = [skw([1 0 0]) zeros(3,1); zeros(1,4)];
  V{2} = [skw([0 1 0]) zeros(3,1); zeros(1,4)];
  V{3} = [skw([0 0 1]) zeros(3,1); zeros(1,4)];

end
